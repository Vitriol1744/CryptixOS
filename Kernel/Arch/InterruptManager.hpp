/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

class InterruptHandler;
namespace InterruptManager
{
    void              InstallExceptions();
    InterruptHandler* AllocateHandler(u8 hint = 0x20 + 0x10);

    void              Mask(u8 irq);
    void              Unmask(u8 irq);

    void              SendEOI(u8 vector);
}; // namespace InterruptManager
