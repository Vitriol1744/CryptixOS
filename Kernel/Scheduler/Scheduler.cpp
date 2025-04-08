/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Scheduler/Scheduler.hpp>

#include <Arch/CPU.hpp>
#include <Arch/InterruptHandler.hpp>
#include <Arch/InterruptManager.hpp>
#include <Arch/x86_64/Drivers/IoApic.hpp>

#include <Memory/PMM.hpp>
#include <Library/Spinlock.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>
#include <Time/Time.hpp>

#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/VFS.hpp>

#include <deque>

u8 g_ScheduleVector = 48;
namespace
{
    Spinlock                            s_Lock{};

    Process*                            s_KernelProcess = nullptr;
    Spinlock                            s_ProcessListLock;
    std::unordered_map<pid_t, Process*> s_Processes;
    ProcFs*                             s_ProcFs = nullptr;
} // namespace

struct ThreadQueue
{
    mutable Spinlock    Lock;
    std::deque<Thread*> Queue;

    constexpr usize     Size() const { return Queue.size(); }
    constexpr bool      IsEmpty() const { return Queue.empty(); }

    auto                begin() { return Queue.begin(); }
    auto                end() { return Queue.end(); }

    inline Thread*      Front() const
    {
        ScopedLock guard(Lock);
        return Queue.front();
    }
    inline Thread* Back() const
    {
        ScopedLock guard(Lock);
        return Queue.back();
    }

    inline void PushBack(Thread* t)
    {
        ScopedLock guard(Lock);
        Queue.push_back(t);
    }
    inline void PushFront(Thread* t)
    {
        ScopedLock guard(Lock);
        Queue.push_front(t);
    }

    inline void PopBack()
    {
        ScopedLock guard(Lock);
        Queue.pop_back();
    }
    inline void PopFront()
    {
        ScopedLock guard(Lock);
        Queue.pop_front();
    }

    inline Thread* PopFrontElement()
    {
        ScopedLock guard(Lock);
        return Queue.pop_front_element();
    }
    inline Thread* PopBackElement()
    {
        ScopedLock guard(Lock);
        return Queue.pop_back_element();
    }

    inline void Erase(Thread** it)
    {
        ScopedLock guard(Lock);
        Queue.erase(it);
    }
};

ThreadQueue s_ExecutionQueue;
ThreadQueue s_WaitQueue;
ThreadQueue s_ReadyQueue;
ThreadQueue s_BlockedQueue;

void        Scheduler::Initialize()
{
    s_KernelProcess = Process::CreateKernelProcess();

    Time::Initialize();
    Time::GetSchedulerTimer()->SetCallback<Tick>();

    LogInfo("Scheduler: Kernel process created");
    LogInfo("Scheduler: Initialized");
}
void Scheduler::InitializeProcFs()
{
    VFS::CreateNode(VFS::GetRootNode(), "/proc", 0755 | S_IFDIR);
    Assert(VFS::Mount(VFS::GetRootNode(), "", "/proc", "procfs"));
    s_ProcFs = reinterpret_cast<ProcFs*>(VFS::GetMountPoints()["/proc"]);

    s_ProcFs->AddProcess(s_KernelProcess);
}
void Scheduler::PrepareAP(bool start)
{
    Process* process    = Process::CreateIdleProcess();

    auto     idleThread = process->CreateThread(
        reinterpret_cast<uintptr_t>(Arch::Halt), false, CPU::GetCurrent()->ID);
    idleThread->SetState(ThreadState::eReady);

    CPU::GetCurrent()->Idle = idleThread;

    if (start)
    {
        CPU::SetInterruptFlag(true);

        CPU::WakeUp(0, true);
        for (;;) Arch::Halt();
    }
};

void Scheduler::Block(Thread* thread)
{
    if (thread->GetState() == ThreadState::eBlocked) return;
    thread->SetState(ThreadState::eBlocked);

    if (thread == Thread::GetCurrent()) Yield(true);
}
void Scheduler::Unblock(Thread* thread)
{
    if (thread->GetState() != ThreadState::eBlocked) return;

    thread->SetState(ThreadState::eReady);
    if (!thread->m_IsEnqueued) EnqueueThread(thread);

    for (auto it = s_BlockedQueue.begin(); it != s_BlockedQueue.end(); it++)
        if (*it == thread)
        {
            s_BlockedQueue.Erase(it);
            break;
        }
#ifdef CTOS_TARGET_X86_64
    Lapic::Instance()->SendIpi(g_ScheduleVector, CPU::GetCurrent()->LapicID);
#endif
}

void Scheduler::Yield(bool saveCtx)
{
    CPU::SetInterruptFlag(false);
#ifdef CTOS_TARGET_X86_64
    Lapic::Instance()->Stop();
#endif

    Thread* currentThread = Thread::GetCurrent();
    if (saveCtx) currentThread->YieldAwaitLock.Acquire();
    else
    {
#ifdef CTOS_TARGET_X86_64
        CPU::SetGSBase(reinterpret_cast<uintptr_t>(CPU::GetCurrent()->Idle));
        CPU::SetKernelGSBase(
            reinterpret_cast<uintptr_t>(CPU::GetCurrent()->Idle));
#endif
    }

#ifdef CTOS_TARGET_X86_64
    Lapic::Instance()->SendIpi(g_ScheduleVector, CPU::GetCurrent()->LapicID);
#endif

    CPU::SetInterruptFlag(true);

    if (saveCtx)
    {
        currentThread->YieldAwaitLock.Acquire();
        currentThread->YieldAwaitLock.Release();
    }
    else
        for (;;) Arch::Halt();
}

Process* Scheduler::GetKernelProcess() { return s_KernelProcess; }

Process* Scheduler::CreateProcess(Process* parent, std::string_view name,
                                  const Credentials& creds)
{
    auto       proc = new Process(parent, name, creds);

    ScopedLock guard(s_ProcessListLock);
    s_Processes[proc->GetPid()] = proc;
    s_ProcFs->AddProcess(proc);

    return proc;
}
void Scheduler::RemoveProcess(pid_t pid)
{
    ScopedLock guard(s_ProcessListLock);
    s_Processes.erase(pid);
    s_ProcFs->RemoveProcess(pid);
}

bool Scheduler::ValidatePid(pid_t pid)
{
    ScopedLock guard(s_ProcessListLock);
    return s_Processes.contains(pid);
}
Process* Scheduler::GetProcess(pid_t pid)
{
    ScopedLock guard(s_ProcessListLock);

    auto       it = s_Processes.find(pid);

    return it != s_Processes.end() ? s_Processes[pid] : nullptr;
}

void Scheduler::EnqueueThread(Thread* thread)
{
    ScopedLock guard(s_Lock);

    if (thread->m_IsEnqueued) return;

    thread->m_IsEnqueued = true;
    thread->SetState(ThreadState::eReady);
    s_ExecutionQueue.PushBack(thread);
}

void Scheduler::EnqueueNotReady(Thread* thread)
{
    ScopedLock guard(s_Lock);

    thread->m_IsEnqueued = true;
    if (thread->GetState() == ThreadState::eRunning)
        thread->SetState(ThreadState::eReady);

    s_ExecutionQueue.PushBack(thread);
}
void Scheduler::DequeueThread(Thread* thread)
{
    if (!thread->IsEnqueued()) return;
    ScopedLock guard(s_Lock);

    for (auto& current : s_ExecutionQueue)
        if (current == thread)
        {
            thread->m_IsEnqueued = false;
            thread->SetState(ThreadState::eDequeued);
        }
}

std::unordered_map<pid_t, Process*>& Scheduler::GetProcessMap()
{
    return s_Processes;
}

Thread* Scheduler::GetNextThread(usize cpuID)
{
    if (s_ExecutionQueue.IsEmpty()) return nullptr;

    auto thread          = s_ExecutionQueue.PopFrontElement();
    thread->m_IsEnqueued = false;

    return thread;
}
Thread* Scheduler::PickReadyThread()
{
    auto newThread = GetNextThread(CPU::GetCurrent()->ID);
    for (; newThread && newThread->GetState() != ThreadState::eReady;)
    {
        if (newThread->IsDead())
        {
            delete newThread;
            continue;
        }
        else if (newThread->IsBlocked())
        {
            s_BlockedQueue.PushBack(newThread);
            newThread = GetNextThread(CPU::GetCurrent()->ID);
            continue;
        }

        EnqueueNotReady(newThread);
        newThread = GetNextThread(CPU::GetCurrent()->ID);
    }

    Thread* currentThread = Thread::GetCurrent();
    if (!newThread)
        return currentThread ? currentThread : CPU::GetCurrent()->Idle;

    newThread->DispatchAnyPendingSignal();
    return newThread;
}
void Scheduler::SwitchContext(Thread* newThread, CPUContext* oldContext)
{
    auto currentThread = CPU::GetCurrentThread();
    if (currentThread) currentThread->YieldAwaitLock.Release();

    if (currentThread && !currentThread->IsDead())
    {
        if (currentThread != CPU::GetCurrent()->Idle)
            EnqueueNotReady(currentThread);

        CPU::SaveThread(currentThread, oldContext);
    }

    CPU::LoadThread(newThread, oldContext);

    if (currentThread && currentThread->IsDead()
        && currentThread != CPU::GetCurrent()->Idle)
        delete currentThread;
}

void Scheduler::Tick(CPUContext* ctx)
{
    auto newThread = PickReadyThread();
    SwitchContext(newThread, ctx);

    if (newThread != CPU::GetCurrent()->Idle)
        newThread->SetState(ThreadState::eRunning);

    CPU::Reschedule(newThread->GetParent()->m_Quantum * 1_ms);
}
