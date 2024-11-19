/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Thread.hpp"

#include "Arch/x86_64/CPU.hpp"

Thread::Thread(Process* parent, uintptr_t pc, uintptr_t arg, i64 runOn)
    : runningOn(runOn)
    , self(this)
    , error(no_error)
    , parent(parent)
{
    CPU::PrepareThread(this, pc, arg);
}
