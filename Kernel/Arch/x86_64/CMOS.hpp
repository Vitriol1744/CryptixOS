/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Arch/x86_64/Types.hpp"

// http://www.bioscentral.com/misc/cmosmap.htm
inline static const byte CMOS_RTC_SECONDS         = 0x00;
inline static const byte CMOS_RTC_MINUTES         = 0x02;
inline static const byte CMOS_RTC_HOURS           = 0x04;
inline static const byte CMOS_RTC_WEEK_DAY        = 0x06;
inline static const byte CMOS_RTC_MONTH_DAY       = 0x07;
inline static const byte CMOS_RTC_MONTH           = 0x08;
inline static const byte CMOS_RTC_YEAR            = 0x09;
inline static const byte CMOS_STATUS_REGISTER_A   = 0x0a;
inline static const byte CMOS_STATUS_REGISTER_B   = 0x0b;
inline static const byte CMOS_STATUS_REGISTER_C   = 0x0c;
inline static const byte CMOS_STATUS_REGISTER_D   = 0x0d;
inline static const byte CMOS_DIAGNOSTIC_STATUS   = 0x0e;
inline static const byte CMOS_SHUTDOWN_STATUS     = 0x0f;
inline static const byte CMOS_FLOPPY_TYPES        = 0x10;
inline static const byte CMOS_SYS_CONFIG_SETTINGS = 0x11;
inline static const byte CMOS_HARD_DISK_TYPES     = 0x12;
inline static const byte CMOS_LOW_MEMORY_L        = 0x15;
inline static const byte CMOS_LOW_MEMORY_H        = 0x16;
inline static const byte CMOS_MIDDLE_MEMORY_L     = 0x17;
inline static const byte CMOS_MIDDLE_MEMORY_H     = 0x18;
inline static const byte CMOS_HIGH_MEMORY_L       = 0x30;
inline static const byte CMOS_HIGH_MEMORY_H       = 0x31;
inline static const byte CMOS_RTC_CENTURY         = 0x32;

namespace CMOS
{
    void Write(byte reg, byte value);
    byte Read(byte reg);
} // namespace CMOS
