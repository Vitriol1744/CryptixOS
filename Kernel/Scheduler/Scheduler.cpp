/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Scheduler.hpp"

#include "Arch/x86_64/CPU.hpp"
#include "Arch/x86_64/IDT.hpp"
#include "Memory/PhysicalMemoryManager.hpp"
#include "Memory/VirtualMemoryManager.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Thread.hpp"

#include <deque>

usize scheduleVector = 0;
namespace Scheduler
{
    inline constexpr uintptr_t   kernelStackSize = 0x10000;  // 64 kib
    inline constexpr uintptr_t   userStackSize   = 0x200000; // 2 mib
    static std::mutex            lock;

    static std::deque<::Thread*> queues[2];
    [[maybe_unused]] static auto active  = &queues[0];
    static auto                  expired = &queues[1];

    static void                  Scheduler(CPUContext* ctx);
    [[maybe_unused]]
    static void SwitchContext(CPUContext* context)
    {
        __asm__ volatile(
            "mov %0, %%rsp\n"
            "pop %%rax\n"
            "pop %%rbx\n"
            "pop %%rcx\n"
            "pop %%rdx\n"
            "pop %%rsi\n"
            "pop %%rdi\n"
            "pop %%rbp\n"
            "pop %%r8\n"
            "pop %%r9\n"
            "pop %%r10\n"
            "pop %%r11\n"
            "pop %%r12\n"
            "pop %%r13\n"
            "pop %%r14\n"
            "pop %%r15\n"

            "add $0x10, %%rsp\n"
            "swapgs\n"
            "sti\n"
            "iretq"
            :
            : "r"(context));
    }

    Thread* GetNextThread(usize cpuID)
    {
        std::unique_lock guard(lock);
        if (active->empty()) std::swap(active, expired);
        if (active->empty()) return nullptr;

        for (auto it = active->rbegin(); it != active->rend(); it++)
        {
            Thread* thread = *it;
            if (thread->runningOn < 0 || thread->runningOn == cpuID)
            {
                active->erase(std::next(it).base());
                thread->enqueued = false;

                return thread;
            }
        }
        return nullptr;
    }

    void Initialize()
    {
        auto& handler = *IDT::AllocateHandler();
        handler.SetHandler(Scheduler);
        scheduleVector = handler.GetInterruptVector();

        LogInfo("Scheduler vector = {}", scheduleVector);
    }
    void PrepareAP(bool start)
    {
        CPU::GetCurrent()->tss.ist[0]
            = ToHigherHalfAddress<uintptr_t>(
                  PhysicalMemoryManager::AllocatePages<uintptr_t>(
                      kernelStackSize / PMM::PAGE_SIZE))
            + kernelStackSize;
        IDT::SetIST(14, 2);

        Process* process = new Process;
        process->pid     = 0;
        process->name    = "Idle Process for CPU: ";
        process->name += std::to_string(CPU::GetCurrent()->id);
        process->pageMap = VMM::GetKernelPageMap();

        auto idleThread  = new ::Thread(
            process, reinterpret_cast<uintptr_t>(Arch::Halt), true);
        CPU::GetCurrent()->idle = idleThread;

        if (start) CPU::WakeUp(0, true);

        Arch::Halt();
    };
    void EnqueueThread(Thread* thread)
    {
        std::unique_lock guard(lock);
        if (thread->enqueued) return;

        thread->state = ThreadState::eReady;
        expired->push_front(thread);
    }
    void EnqueueNotReady(Thread* thread)
    {
        std::unique_lock guard(lock);

        thread->enqueued = true;
        if (thread->state == ThreadState::eRunning)
            thread->state = ThreadState::eReady;

        expired->push_front(thread);
    }

    static void Scheduler(CPUContext* ctx)
    {
        auto newThread = GetNextThread(CPU::GetCurrentID());
        while (newThread && newThread->state != ThreadState::eReady)
        {
            if (newThread->state == ThreadState::eKilled)
            {
                delete newThread;
                continue;
            }

            EnqueueNotReady(newThread);
            newThread = GetNextThread(CPU::GetCurrentID());
        }

        if (!newThread) newThread = CPU::GetCurrent()->idle;
        else newThread->state = ThreadState::eRunning;

        auto currentThread = CPU::GetCurrent()->currentThread;
        if (currentThread && currentThread->state != ThreadState::eKilled)
        {
            if (currentThread != CPU::GetCurrent()->idle)
                EnqueueNotReady(currentThread);

            CPU::SaveThread(currentThread, ctx);
        }

        CPU::Reschedule(1);
        CPU::LoadThread(newThread, ctx);
        if (currentThread && currentThread->state == ThreadState::eKilled
            && currentThread != CPU::GetCurrent()->idle)
            delete currentThread;
    }
}; // namespace Scheduler
