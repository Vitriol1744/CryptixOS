/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Arch/InterruptHandler.hpp>

constexpr usize DPL_RING0 = 0x00;
constexpr usize DPL_RING3 = 0x03;

namespace IDT
{
    void              Initialize();
    void              Load();

    InterruptHandler* AllocateHandler(u8 hint = 0x20 + 0x10);
    InterruptHandler* GetHandler(u8 vector);

    void              SetIST(u8 vector, u32 value);
    void              SetDPL(u8 vector, u8 dpl = 3);
}; // namespace IDT
