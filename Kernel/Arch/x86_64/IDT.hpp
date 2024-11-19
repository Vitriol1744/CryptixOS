/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Arch/Interrupts/InterruptHandler.hpp"
#include "Utility/Types.hpp"
#include <stdlib.h>

inline constexpr const usize DPL_RING0 = 0x00;
inline constexpr const usize DPL_RING3 = 0x03;

// TODO(v1tr10l7): Move it somewhere  else

namespace IDT
{
    void              Initialize();
    void              Load();

    void              SetIST(u8 vector, u32 value);

    InterruptHandler* AllocateHandler(u8 hint = 0x20 + 16);
    InterruptHandler& GetHandler(u8 vector);
    void              RegisterInterruptHandler(InterruptHandler* handler,
                                               uint8_t           dpl = DPL_RING0);
} // namespace IDT

struct CPUContext
{
    u64 ds;
    u64 es;

    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 interruptVector;
    u64 errorCode;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};
