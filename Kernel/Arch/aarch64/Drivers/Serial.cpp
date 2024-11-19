/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Drivers/Serial.hpp"

#include "Arch/aarch64/MMIO.hpp"
#include "Utility/BootInfo.hpp"

#include "Memory/VirtualMemoryManager.hpp"

namespace Serial
{
    constexpr const uintptr_t UART_BASE     = 0x9000000;
    static uintptr_t          s_UartAddress = UART_BASE;

    bool                      Initialize()
    {
        uintptr_t virt
            = VMM::AllocateSpace(vsptypes::other, 0x1000, sizeof(u16));
        VMM::GetKernelPageMap()->Map(virt, UART_BASE, Attributes::eRW,
                                     CachingMode::eUncacheableStrong);
        s_UartAddress = virt;

        s_UartAddress += BootInfo::GetHHDMOffset();
        // Disable the UART.
        MMIO::Out<u16>(s_UartAddress + 0x30, 0);

        // Set word length to 8 bits and enable FIFOs
        MMIO::Out<u16>(s_UartAddress + 0x2C, (3 << 5) | (1 << 4));

        // Enable UART, TX and RX
        MMIO::Out<u16>(s_UartAddress + 0x30, (1 << 0) | (1 << 8) | (1 << 9));

        return true;
    }

    u8 Read()
    {
        while (MMIO::In<u16>(s_UartAddress + 0x18) & (1 << 4))
            asm volatile("isb" ::: "memory");
        // Arch::Pause();

        return MMIO::In<u8>(s_UartAddress);
    }
    void Write(u8 byte)
    {
        while (MMIO::In<u16>(s_UartAddress + 0x18) & (1 << 5))
            asm volatile("isb" ::: "memory");

        MMIO::Out<u8>(s_UartAddress, byte);
    }
} // namespace Serial
