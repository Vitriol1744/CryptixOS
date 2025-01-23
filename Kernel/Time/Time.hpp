/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Utility/Types.hpp>

namespace Time
{
    usize    GetEpoch();
    timespec GetReal();
    timespec GetMonotonic();

    void     Tick(usize ns);
} // namespace Time
