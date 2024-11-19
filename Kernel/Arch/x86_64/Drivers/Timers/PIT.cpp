/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "PIT.hpp"

#include "Common.hpp"

#include "Arch/Interrupts/InterruptHandler.hpp"
#include "Arch/x86_64/Drivers/PIC.hpp"
#include "Arch/x86_64/IDT.hpp"
#include "Arch/x86_64/IO.hpp"

#include <atomic>

namespace PIT
{
    static constexpr const u64 PIT_FREQUENCY = 1000;
    static std::atomic<u64>    tick          = 0;
    static u8                  timerVector   = 0;

    [[maybe_unused]]
    static void TimerTick(struct CPUContext* ctx)
    {
        tick++;
        LogTrace("Timer Handler");
    }

    void Initialize()
    {
        LogTrace("PIT: Initializing...");
        SetFrequency(PIT_FREQUENCY);

        [[maybe_unused]] InterruptHandler* handler = IDT::AllocateHandler(0x20);
        handler->SetHandler([](::CPUContext* ctx) { TimerTick(ctx); });
        PIC::UnmaskIRQ(handler->GetInterruptVector() - 0x20);
        LogInfo("PIT: Timer vector = {} ", timerVector);
    }

    void SetFrequency(usize frequency)
    {
        u64 reloadValue = PIT_BASE_FREQUENCY / frequency;
        if (PIT_BASE_FREQUENCY % frequency > frequency / 2) reloadValue++;
        SetReloadValue(reloadValue);
    }
    void SetReloadValue(u16 reloadValue)
    {
        IO::Out<byte>(0x43, 0x34);
        IO::Out<byte>(0x40, static_cast<byte>(reloadValue));
        IO::Out<byte>(0x40, static_cast<byte>(reloadValue >> 8));
    }
}; // namespace PIT
