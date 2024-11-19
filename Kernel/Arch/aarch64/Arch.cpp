/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Arch/Arch.hpp"

// TODO(V1tri0l1744): Aarch64::Arch::LogE9
namespace Arch
{
    void                           Initialize() {}

    __attribute__((noreturn)) void Halt()
    {
        for (;;) __asm__ volatile("msr daifclr, #0b1111; wfi");
    }
    void Pause() { __asm__ volatile("isb" ::: "memory"); }
} // namespace Arch
