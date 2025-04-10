/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/UnixTypes.hpp>

using DeviceMajor = u32;
using DeviceMinor = u32;

constexpr dev_t MakeDevice(DeviceMajor major, DeviceMinor minor)
{
    dev_t dev = dev_t(major & 0x00000fffu) << 8;
    dev |= dev_t(major & 0xfffff000u) << 32;
    dev |= dev_t(minor & 0x000000ffu);
    dev |= dev_t(minor & 0xffffff00u) << 12;

    return dev;
}

class Device
{
  public:
    Device(DeviceMajor major, DeviceMinor minor)
        : id(MakeDevice(major, minor))
    {
    }

    inline dev_t             GetID() const noexcept { return id; }
    virtual std::string_view GetName() const noexcept = 0;

    virtual const stat&      GetStats() { return m_Stats; }

    virtual isize            Read(void* dest, off_t offset, usize bytes) = 0;
    virtual isize Write(const void* src, off_t offset, usize bytes)      = 0;

    virtual i32   IoCtl(usize request, uintptr_t argp)                   = 0;

  protected:
    dev_t id;
    stat  m_Stats;
};
