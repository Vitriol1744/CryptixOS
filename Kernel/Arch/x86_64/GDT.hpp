/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

struct TaskStateSegment
{
    CTOS_UNUSED u32 reserved1;
    CTOS_UNUSED u64 rsp[3];
    CTOS_UNUSED u64 reserved2;
    CTOS_UNUSED u64 ist[7];
    CTOS_UNUSED u64 reserved3;
    CTOS_UNUSED u16 reserved4;
    CTOS_UNUSED u16 ioMapBase;
} __attribute__((packed));

namespace GDT
{
    inline static constexpr const usize KERNEL_CODE_SELECTOR   = 0x08ull;
    inline static constexpr const usize KERNEL_DATA_SELECTOR   = 0x10ull;
    inline static constexpr const usize USERLAND_CODE_SELECTOR = 0x18ull | 3ull;
    inline static constexpr const usize USERLAND_DATA_SELECTOR = 020ull | 3ull;

    void                                Initialize(u64 id);
    void                                Load();

    void                                LoadTSS(TaskStateSegment* tss);
}; // namespace GDT
