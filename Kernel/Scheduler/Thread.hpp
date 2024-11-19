/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Arch/x86_64/IDT.hpp"

#include "Scheduler/Scheduler.hpp"

#include <errno.h>
#include <vector>

using tid_t = i64;

enum class ThreadState
{
    eReady,
    eIdle,
    eKilled,
    eRunning,
};

struct Thread
{
    Thread() = default;
    Thread(struct Process* parent, uintptr_t pc, uintptr_t arg, i64 runOn = -1);
    usize     runningOn;
    Thread*   self;
    uintptr_t stack;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    uintptr_t kstack;
    uintptr_t pfstack;
#endif

    tid_t                                    tid;
    errno_t                                  error;
    struct Process*                          parent;

    CPUContext                               ctx;
    CPUContext                               savedCtx;

    std::vector<std::pair<uintptr_t, usize>> stacks;
    bool                                     user = false;

#if defined(__x86_64__)
    uintptr_t gsBase;
    uintptr_t fsBase;

    size_t    fpuStoragePages;
    uint8_t*  fpuStorage;
#elif defined(__aarch64__)
    uintptr_t el0Base;
#endif

    bool        enqueued = false;
    ThreadState state    = ThreadState::eIdle;
};
