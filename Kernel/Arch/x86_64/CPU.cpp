/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "CPU.hpp"

#include "Arch/x86_64/CPUID.hpp"
#include "Arch/x86_64/GDT.hpp"
#include "Arch/x86_64/IDT.hpp"

#include "Utility/Math.hpp"

#include "Memory/VirtualMemoryManager.hpp"
#include "Scheduler/Process.hpp"
#include "Scheduler/Scheduler.hpp"
#include "Scheduler/Thread.hpp"

#include <mutex>
#include <vector>
inline static u64                  CPU_MSR_STAR    = 0xc0000081;
inline static u64                  CPU_MSR_LSTAR   = 0xc0000082;
[[maybe_unused]] inline static u64 CPU_MSR_CSTAR   = 0xc0000083;
inline static u64                  CPU_MSR_SFMASK  = 0xc0000084;
inline constexpr uintptr_t         kernelStackSize = 0x10000;  // 64 kib
inline constexpr uintptr_t         userStackSize   = 0x200000; // 2 mib

__attribute__((noreturn)) void     syscallEntry() { Arch::Halt(); }

namespace
{
    usize                             bspID          = 0;
    usize                             fpuStorageSize = 0;
    [[maybe_unused]] std::atomic<u64> onlineAPs      = 0;
    [[maybe_unused]] bool             sysenter       = false;
    [[maybe_unused]] bool             syscall        = false;

    inline void                       XSave(uintptr_t ctx)
    {
        return;
        asm volatile("xsave (%0)"
                     :
                     : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                     : "memory");
    }

    inline void XRestore(uintptr_t ctx)
    {
        return;
        __asm__ volatile("xrstor (%0)"
                         :
                         : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                         : "memory");
    }

    inline void FXSave(uintptr_t ctx)
    {
        return;
        __asm__ volatile("fxsave (%0)" : : "r"(ctx) : "memory");
    }

    inline void FXRestore(uintptr_t ctx)
    {
        return;
        __asm__ volatile("fxrstor (%0)" : : "r"(ctx) : "memory");
    }
}; // namespace

std::vector<CPU> CPU::cpus;
FPUSaveFunc      CPU::fpuSave    = nullptr;
FPURestoreFunc   CPU::fpuRestore = nullptr;

void             CPU::InitializeBSP()
{
    limine_smp_response* smp      = BootInfo::GetSMP_Response();
    u64                  cpuCount = smp->cpu_count;

    cpus.resize(cpuCount);
    bspID = smp->bsp_lapic_id;

    LogTrace("BSP: Initializing...");
    for (usize i = 0; i < cpuCount; i++)
    {
        limine_smp_info* smpInfo = smp->cpus[i];
        if (smpInfo->lapic_id != bspID) continue;

        smpInfo->extra_argument = reinterpret_cast<u64>(&cpus[i]);
        cpus[i].archID          = smpInfo->lapic_id;
        cpus[i].id              = i;

        GDT::Initialize(bspID);
        IDT::Initialize();
        IDT::Load();

        CPU::SetKernelGSBase(smpInfo->extra_argument);
        CPU::SetGSBase(smpInfo->extra_argument);

        cpus[i].lapic.Initialize();
        Scheduler::Initialize();
        LogInfo("BSP: Initialized");
    }
};

static void InitializeCPU(limine_smp_info* cpu)
{
    CPU* current = reinterpret_cast<CPU*>(cpu->extra_argument);
    if (current->archID != bspID)
    {
        CPU::EnablePAT();
        VMM::GetKernelPageMap()->Load();

        GDT::Initialize(current->id);
        IDT::Load();

        CPU::SetKernelGSBase(cpu->extra_argument);
        CPU::SetGSBase(cpu->extra_argument);
    }

    CPU::EnableSSE();
    CPU::EnableSMEP();
    CPU::EnableSMAP();
    CPU::EnableUMIP();

    u64 rax, rbx, rcx, rdx;
    if (CPU::ID(1, 0, rax, rbx, rcx, rdx) && (rcx & CPU_FEAT_ECX_XSAVE))
    {
        Assert(cpu != nullptr);
        if (current->archID == bspID) LogInfo("FPU: xsave supported");

        // Enable XSAVE and x{get,set}bv
        CPU::WriteCR4(CPU::ReadCR4() | CPU_CR4_OSXSAVE);

        u64 xcr0 = 0;
        if (current->archID == bspID)
            LogInfo("FPU: Saving x87 state using xsave");
        xcr0 |= CPU_XCR0_X87;
        if (current->archID == bspID)
            LogInfo("FPU: Saving SSE state using xsave");
        xcr0 |= CPU_XCR0_SSE;

        if (rcx & CPU_FEAT_ECX_AVX)
        {
            if (current->archID == bspID)
                LogInfo("FPU: Saving AVX state using xsave");
            xcr0 |= CPU_XCR0_AVX;
        }

        if (CPU::ID(7, 0, rax, rbx, rcx, rdx) && (rbx & CPU_FEAT_EBX_AVX512))
        {
            if (current->archID == bspID)
                LogInfo("FPU: Saving AVX-512 state using xsave");
            xcr0 |= CPU_XCR0_OPMASK;
            xcr0 |= CPU_XCR0_ZMM_HI256;
            xcr0 |= CPU_XCR0_ZMM_HI16;
        }

        CPU::WriteXCR(0, xcr0);

        if (!CPU::ID(0xd, 0, rax, rbx, rcx, rdx)) Panic("CPUID failure");

        fpuStorageSize  = rcx;
        CPU::fpuSave    = XSave;
        CPU::fpuRestore = XRestore;
    }
    else
    {
        if (current->archID == bspID)
            LogInfo("FPU: Falling back to legacy fxsave");
        fpuStorageSize  = 512;
        CPU::fpuSave    = FXSave;
        CPU::fpuRestore = FXRestore;
    }

    CPU::WriteMSR(0xC0000080,
                  CPU::ReadMSR(0xC0000080)
                      | (1 << 0)); // IA32_EFER enable syscall
    CPU::WriteMSR(CPU_MSR_STAR,
                  ((uint64_t(GDT::KERNEL_DATA_SELECTOR) | 0x03) << 48)
                      | (uint64_t(GDT::KERNEL_CODE_SELECTOR)
                         << 32)); // IA32_STAR ss and cs
    CPU::WriteMSR(
        CPU_MSR_LSTAR,
        reinterpret_cast<uint64_t>(syscallEntry)); // IA32_LSTAR handler
    CPU::WriteMSR(CPU_MSR_SFMASK, ~uint32_t(2));   // IA32_FMASK rflags mask

    if (current->archID != bspID) current->lapic.Initialize();
}

void CPU::StartAPs()
{
    LogTrace("SMP: Launching APs");

    auto cpuEntry = [](limine_smp_info* cpu)
    {
        CPU*              current = reinterpret_cast<CPU*>(cpu->extra_argument);
        static std::mutex lock;
        {
            std::unique_lock guard(lock);
            InitializeCPU(cpu);

            LogInfo("SMP: CPU {} is up", current->id);
            current->isOnline = true;
        }

        if (bspID != current->archID) Scheduler::PrepareAP();
    };

    auto smpResponse = BootInfo::GetSMP_Response();
    for (usize i = 0; i < smpResponse->cpu_count; i++)
    {
        auto smpInfo            = smpResponse->cpus[i];
        smpInfo->extra_argument = reinterpret_cast<uint64_t>(&cpus[i]);
        cpus[i].archID          = smpInfo->lapic_id;
        cpus[i].id              = i;

        if (bspID != cpus[i].archID)
        {
            smpInfo->goto_address = cpuEntry;
            while (cpus[i].isOnline == false) Arch::Pause();
        }
        else cpuEntry(smpInfo);
    }
}

void CPU::PrepareThread(Thread* thread, uintptr_t pc, uintptr_t arg)
{
    thread->ctx.rflags = 0x202;
    thread->ctx.rip    = pc;
    thread->ctx.rdi    = arg;

    thread->fpuStoragePages
        = Math::DivRoundUp(CPU::GetCurrent()->fpuStorageSize, PMM::PAGE_SIZE);

    uintptr_t fpuStorage
        = PMM::AllocatePages<uintptr_t>(thread->fpuStoragePages);

    thread->fpuStorage = ToHigherHalfAddress<uint8_t*>(fpuStorage);

    uintptr_t pkstack
        = PMM::AllocatePages<uintptr_t>(kernelStackSize / PMM::PAGE_SIZE);
    thread->kstack = ToHigherHalfAddress<uintptr_t>(pkstack) + kernelStackSize;
    thread->stacks.push_back(std::make_pair(pkstack, kernelStackSize));

    uintptr_t ppfstack
        = PMM::AllocatePages<uintptr_t>(kernelStackSize / PMM::PAGE_SIZE);
    thread->pfstack
        = ToHigherHalfAddress<uintptr_t>(ppfstack) + kernelStackSize;
    thread->stacks.push_back(std::make_pair(ppfstack, kernelStackSize));

    thread->gsBase = reinterpret_cast<uintptr_t>(thread);
    if (thread->user == true)
    {
        thread->ctx.cs  = GDT::USERLAND_CODE_SELECTOR | 0x03;
        thread->ctx.ss  = GDT::USERLAND_DATA_SELECTOR | 0x03;

        thread->ctx.rsp = thread->stack;

        CPU::GetCurrent()->fpuRestore(
            reinterpret_cast<uintptr_t>(thread->fpuStorage));

        u16 defaultFCW = 0b1100111111;
        asm volatile("fldcw %0" ::"m"(defaultFCW) : "memory");
        u32 defaultMXCSR = 0b1111110000000;
        asm volatile("ldmxcsr %0" ::"m"(defaultMXCSR) : "memory");

        CPU::GetCurrent()->fpuSave(
            reinterpret_cast<uintptr_t>(thread->fpuStorage));
    }
    else
    {
        thread->ctx.cs  = GDT::KERNEL_CODE_SELECTOR;
        thread->ctx.ss  = GDT::KERNEL_DATA_SELECTOR;

        thread->ctx.rsp = thread->stack = thread->kstack;
    }
}

void CPU::SaveThread(Thread* thread, CPUContext* ctx)
{
    thread->ctx    = *ctx;

    thread->gsBase = CPU::GetKernelGSBase();
    thread->fsBase = CPU::GetFSBase();

    fpuSave(reinterpret_cast<uintptr_t>(thread->fpuStorage));
}
void CPU::LoadThread(Thread* thread, CPUContext* ctx)
{
    thread->runningOn             = CPU::GetCurrent()->id;

    CPU::GetCurrent()->tss.ist[1] = thread->pfstack;
    fpuRestore(reinterpret_cast<uintptr_t>(thread->fpuStorage));
    thread->parent->pageMap->Load();

    CPU::SetGSBase(reinterpret_cast<u64>(thread));
    CPU::SetKernelGSBase(thread->gsBase);
    CPU::SetFSBase(thread->fsBase);

    *ctx = thread->ctx;
}
void CPU::Reschedule(usize ms)
{
    CPU::GetCurrent()->lapic.Start(scheduleVector, ms, Lapic::Mode::eOneshot);
}

void CPU::EnablePAT()
{

    // write-combining/write-protect
    u64 pat = ReadMSR(0x277);
    pat &= 0xffffffff;
    pat |= (u64)0x0105 << 32;
    WriteMSR(0x277, pat);
}
void CPU::EnableSSE()
{
    u64 cr0 = CPU::ReadCR0();
    cr0 &= ~CPU_CR3_EM;
    cr0 |= CPU_CR3_MP;
    CPU::WriteCR0(cr0);

    u64 cr4 = CPU::ReadCR4();
    cr4 |= CPU_CR4_OSFXSR;
    CPU::WriteCR4(cr4);
}
void CPU::EnableSMEP()
{
    u64 rax = 0, rbx = 0, rcx = 0, rdx = 0;
    if (ID(7, 0, rax, rbx, rcx, rdx) && rbx & CPU_FEAT_EBX_SMEP)
        WriteCR4(ReadCR4() | CPU_CR4_SMEP);
}
void CPU::EnableSMAP()
{
    u64 rax = 0, rbx = 0, rcx = 0, rdx = 0;
    if (ID(7, 0, rax, rbx, rcx, rdx) && rbx & CPU_FEAT_EBX_SMAP)
    {
        WriteCR4(ReadCR4() | CPU_CR4_SMAP);
        Clac();
    }
}
void CPU::EnableUMIP()
{
    u64 rax = 0, rbx = 0, rcx = 0, rdx = 0;
    if (ID(7, 0, rax, rbx, rcx, rdx) && rcx & CPU_FEAT_ECX_UMIP)
        WriteCR4(ReadCR4() | CPU_CR4_UMIP);
}

u32 CPU::GetOnlineAPsCount() { return onlineAPs; }
// u64 CPU::GetFPUStorageSize() { return fpuStorageSize; }
