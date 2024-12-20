/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

class Lapic
{
  public:
    enum class Mode : u8
    {
        eOneshot     = 0,
        ePeriodic    = 1,
        eTscDeadline = 2,
    };

    void        Initialize();
    void        SendIpi(u32 flags, u32 id);
    void        SendEOI();

    static void PanicIpi();
    void        Start(u8 vector, u64 ms, Mode mode);

  private:
    u32       id          = 0;
    u32       ticksPerMs  = 0;
    uintptr_t baseAddress = 0;
    bool      x2apic      = false;

    u32       Read(u32 reg);
    void      Write(u32 reg, u64 value);

    void      CalibrateTimer();
};
