/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

#include <unordered_map>

class Process;
struct Thread;
namespace Scheduler
{
    void Initialize();
    void PrepareAP(bool start = false);

    [[noreturn]]
    void                                 Yield();

    Process*                             GetKernelProcess();
    std::unordered_map<pid_t, Process*>& GetProcessList();
    bool                                 ProcessExist(pid_t pid);

    Thread* CreateKernelThread(uintptr_t pc, uintptr_t arg,
                               usize runningOn = -1);

    void    EnqueueThread(Thread* thread);
    void    EnqueueNotReady(Thread* thread);
}; // namespace Scheduler
