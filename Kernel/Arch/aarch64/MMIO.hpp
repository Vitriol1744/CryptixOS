/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <concepts>

namespace MMIO
{
    template <std::unsigned_integral T>
    static inline T In(auto addr)
    {
        volatile auto ptr = reinterpret_cast<volatile T*>(addr);
        return *ptr;
    }

    template <std::unsigned_integral T>
    static inline void Out(auto addr, T value)
    {
        volatile auto ptr = reinterpret_cast<volatile T*>(addr);
        *ptr              = value;
    }
}; // namespace MMIO
