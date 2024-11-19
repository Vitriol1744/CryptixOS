/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

class InterruptHandler;
namespace InterruptManager
{
    void Initialize();
    void RegisterInterruptHandler(InterruptHandler& interruptHandler);
} // namespace InterruptManager
