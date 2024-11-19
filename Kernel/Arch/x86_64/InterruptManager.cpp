/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Arch/Interrupts/InterruptHandler.hpp"

#include "Arch/x86_64/IDT.hpp"

namespace InterruptManager
{
    void Initialize()
    {
        IDT::Initialize();
        IDT::Load();
    }
    void RegisterInterruptHandler(InterruptHandler& interruptHandler)
    {
        IDT::RegisterInterruptHandler(&interruptHandler);
    }
}; // namespace InterruptManager
