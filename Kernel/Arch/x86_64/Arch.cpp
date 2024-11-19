/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Arch/Arch.hpp"

#include "Arch/Interrupts/InterruptManager.hpp"
#include "Arch/x86_64/CPU.hpp"
#include "Arch/x86_64/Drivers/IoApic.hpp"
#include "Arch/x86_64/Drivers/PIC.hpp"
#include "Arch/x86_64/Drivers/Timers/PIT.hpp"
#include "Arch/x86_64/GDT.hpp"

namespace Arch
{
    void Initialize()
    {
        CPU::InitializeBSP();

        PIC::Remap(0x20, 0x28);
        IoApic::Initialize();

        PIT::Initialize();
        CPU::StartAPs();
    }

    __attribute__((noreturn)) void Halt()
    {
        __asm__ volatile("cli;hlt");
        CTOS_ASSERT_NOT_REACHED();
    }
    void Pause() { __asm__ volatile("pause"); }
}; // namespace Arch
