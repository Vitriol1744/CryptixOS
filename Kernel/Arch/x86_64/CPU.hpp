/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Arch/x86_64/Drivers/Timers/Lapic.hpp"
#include "Arch/x86_64/GDT.hpp"

// #include "Scheduler/Thread.hpp"
struct Thread;

#include <cerrno>
#include <vector>

struct CPUContext;

extern usize                scheduleVector;

inline constexpr const u32  CPU_CR3_MP         = BIT(1);
inline constexpr const u32  CPU_CR3_EM         = BIT(2);

inline constexpr const u32  CPU_CR4_VMA        = BIT(0);
inline constexpr const u32  CPU_CR4_PVI        = BIT(1);
inline constexpr const u32  CPU_CR4_TSD        = BIT(2);
inline constexpr const u32  CPU_CR4_DE         = BIT(3);
inline constexpr const u32  CPU_CR4_PSE        = BIT(4);
inline constexpr const u32  CPU_CR4_PAE        = BIT(5);
inline constexpr const u32  CPU_CR4_MCE        = BIT(6);
inline constexpr const u32  CPU_CR4_PGE        = BIT(7);
inline constexpr const u32  CPU_CR4_PCE        = BIT(8);
inline constexpr const u32  CPU_CR4_OSFXSR     = BIT(9);
inline constexpr const u32  CPU_CR4_OSXMMEXCPT = BIT(10);
inline constexpr const u32  CPU_CR4_UMIP       = BIT(11);
inline constexpr const u32  CPU_CR4_VMXE       = BIT(13);
inline constexpr const u32  CPU_CR4_SMXE       = BIT(14);
inline constexpr const u32  CPU_CR4_FSGSBASE   = BIT(16);
inline constexpr const u32  CPU_CR4_PCIDE      = BIT(17);
inline constexpr const u32  CPU_CR4_OSXSAVE    = BIT(18);
inline constexpr const u32  CPU_CR4_SMEP       = BIT(20);
inline constexpr const u32  CPU_CR4_SMAP       = BIT(21);
inline constexpr const u32  CPU_CR4_PKE        = BIT(22);
inline constexpr const u32  CPU_CR4_CET        = BIT(23);
inline constexpr const u32  CPU_CR4_PKS        = BIT(24);

inline constexpr const u32  CPU_XCR0_X87       = BIT(0);
inline constexpr const u32  CPU_XCR0_SSE       = BIT(1);
inline constexpr const u32  CPU_XCR0_AVX       = BIT(2);
inline constexpr const u32  CPU_XCR0_BNDREG    = BIT(3);
inline constexpr const u32  CPU_XCR0_BNDCSR    = BIT(4);
inline constexpr const u32  CPU_XCR0_OPMASK    = BIT(5);
inline constexpr const u32  CPU_XCR0_ZMM_HI256 = BIT(6);
inline constexpr const u32  CPU_XCR0_ZMM_HI16  = BIT(7);
inline constexpr const u32  CPU_XCR0_PKRU      = BIT(9);

using FPUSaveFunc                              = void    (*)(uintptr_t ctx);
using FPURestoreFunc                           = void (*)(uintptr_t ctx);

class CPU
{
  public:
    static void        InitializeBSP();
    static void        StartAPs();

    inline static bool ID(u64 leaf, u64 subleaf, u64& rax, u64& rbx, u64& rcx,
                          u64& rdx)
    {
        u32 cpuidMax;
        asm volatile("cpuid"
                     : "=a"(cpuidMax)
                     : "a"(leaf & 0x80000000)
                     : "rbx", "rcx", "rdx");
        if (leaf > cpuidMax) return false;

        asm volatile("cpuid"
                     : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx)
                     : "a"(leaf), "c"(subleaf));
        return true;
    }
    template <bool flag>
    __attribute__((always_inline)) inline static void SetInterruptFlag()
    {
        if constexpr (flag) __asm__ volatile("sti");
        else __asm__ volatile("cli");
    }
    __attribute__((always_inline)) inline static bool
    SetInterruptFlag(bool flag)
    {
        bool ret = GetInterruptFlag();

        if (flag) __asm__ volatile("sti");
        else __asm__ volatile("cli");

        return ret;
    }

    inline static bool GetInterruptFlag()
    {
        u64 rflags;
        __asm__ volatile("pushf\n\tpop %0" : "=m"(rflags));

        return rflags & (1 << 9);
    }
    inline static void Halt() { __asm__ volatile("hlt"); }

    inline static void WriteMSR(u32 msr, u64 value)
    {
        const u64 rdx = value >> 32;
        const u64 rax = value;
        __asm__ volatile("wrmsr" : : "a"(rax), "d"(rdx), "c"(msr) : "memory");
    }
    inline static u64 ReadMSR(u32 msr)
    {
        u64 rdx = 0;
        u64 rax = 0;
        __asm__ volatile("rdmsr" : "=a"(rax), "=d"(rdx) : "c"(msr) : "memory");
        return (rdx << 32) | rax;
    }

    inline static void WriteXCR(u64 reg, u64 value)
    {
        u32 a = value;
        u32 d = value >> 32;
        asm volatile("xsetbv" ::"a"(a), "d"(d), "c"(reg) : "memory");
    }
    inline static u64 ReadCR0()
    {
        u64 ret;
        asm volatile("mov %%cr0, %0" : "=r"(ret)::"memory");

        return ret;
    }

    inline static u64 ReadCR2()
    {
        u64 ret;
        asm volatile("mov %%cr2, %0" : "=r"(ret)::"memory");

        return ret;
    }

    inline static u64 ReadCR3()
    {
        u64 ret;
        asm volatile("mov %%cr3, %0" : "=r"(ret)::"memory");

        return ret;
    }
    inline static void WriteCR0(u64 value)
    {
        asm volatile("mov %0, %%cr0" ::"r"(value) : "memory");
    }

    inline static void WriteCR2(u64 value)
    {
        asm volatile("mov %0, %%cr2" ::"r"(value) : "memory");
    }
    inline static void WriteCR3(u64 value)
    {
        asm volatile("mov %0, %%cr3" ::"r"(value) : "memory");
    }

    inline static void WriteCR4(u64 value)
    {
        asm volatile("mov %0, %%cr4" ::"r"(value) : "memory");
    }

    inline static u64 ReadCR4()
    {
        u64 ret;
        asm volatile("mov %%cr4, %0" : "=r"(ret)::"memory");

        return ret;
    }
    inline static void Stac()
    {
        if (ReadCR4() & BIT(21)) __asm__ volatile("stac" ::: "cc");
    }
    inline static void Clac()
    {
        if (ReadCR4() & BIT(21)) __asm__ volatile("clac" ::: "cc");
    }

    inline static void SetFSBase(uintptr_t address)
    {
        WriteMSR(0xc0000100, address);
    }
    inline static void SetGSBase(uintptr_t address)
    {
        WriteMSR(0xc0000101, address);
    }
    inline static void SetKernelGSBase(uintptr_t address)
    {
        WriteMSR(0xc0000102, address);
    }

    static inline uintptr_t GetFSBase() { return ReadMSR(0xc0000100); }
    static inline uintptr_t GetGSBase() { return ReadMSR(0xc0000101); }
    inline static uintptr_t GetKernelGSBase() { return ReadMSR(0xc0000102); }
    inline static std::vector<CPU>& GetCPUs() { return cpus; }
    inline static CPU&              GetBSP() { return cpus[0]; }
    inline static CPU*              GetCurrent()
    {
        usize id;
        asm volatile("mov %%gs:0, %0" : "=r"(id)::"memory");

        return &cpus[id];
    }
    /*static inline Thread* GetCurrentThread()
    {
        Thread* currentThread;
        asm volatile("mov %%gs:8, %0" : "=r"(currentThread)::"memory");

        return currentThread;
    }*/

    static void PrepareThread(Thread* thread, uintptr_t pc, uintptr_t arg);

    static void SaveThread(Thread* thread, CPUContext* ctx);
    static void LoadThread(Thread* thread, CPUContext* ctx);
    static void Reschedule(usize ms);

    inline static u64 GetCurrentID()
    {
        return GetOnlineAPsCount() != 0 ? GetCurrent()->id : 0;
    }

    inline static void WakeUp(usize id, bool everyone)
    {
        if (everyone)
            GetCurrent()->lapic.SendIpi(scheduleVector | (0b10 << 18), 0);
        else
            GetCurrent()->lapic.SendIpi(scheduleVector | GetCurrent()->archID,
                                        0);
    }

    static void             EnablePAT();
    static void             EnableSSE();
    static void             EnableSMEP();
    static void             EnableSMAP();
    static void             EnableUMIP();

    static u32              GetOnlineAPsCount();
    static u64              GetFPUStorageSize();

    static std::vector<CPU> cpus;

    static FPUSaveFunc      fpuSave;
    static FPURestoreFunc   fpuRestore;

    // static FPUSaveFunc      fpuSave;
    // static FPURestoreFunc   fpuRestore;

    [[maybe_unused]] usize  id     = 0;
    [[maybe_unused]] void*  empty  = nullptr;

    [[maybe_unused]] u64    archID = 0;
    Lapic                   lapic;
    u64                     lapicTicksPerMs = 0;
    u64                     tscTicksPerNs   = 0;
    bool                    isOnline        = false;
    TaskStateSegment        tss             = {};
    usize                   fpuStorageSize  = 0;

    errno_t                 error;
    struct Thread*          idle;
    Thread*                 currentThread;
};
