/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Prism/Types.hpp"

#include <string_view>

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #define CTOS_ARCH_STRING "x86-64"
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #define CTOS_ARCH_STRING "aarch64"
#endif

namespace Arch
{
    void                           Initialize();

    __attribute__((noreturn)) void Halt();
    void                           Pause();

    void                           PowerOff();
    void                           Reboot();

    usize                          GetEpoch();
}; // namespace Arch
