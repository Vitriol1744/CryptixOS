/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Memory/VMM.hpp"

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include "Arch/x86_64/CPU.hpp"
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #include "Arch/aarch64/CPU.hpp"
#endif

struct Thread;
struct CPUContext;
namespace CPU
{
    constexpr usize KERNEL_STACK_SIZE = 64_kib;
    constexpr usize USER_STACK_SIZE   = 2_mib;

    bool            GetInterruptFlag();
    void            SetInterruptFlag(bool enabled);

    struct CPU;
    CPU*    GetCurrent();
    Thread* GetCurrentThread();

    void    PrepareThread(Thread* thread, uintptr_t pc);

    void    SaveThread(Thread* thread, CPUContext* ctx);
    void    LoadThread(Thread* thread, CPUContext* ctx);
}; // namespace CPU
