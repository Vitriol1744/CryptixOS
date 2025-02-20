/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <cerrno>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <fmt/format.h>
#include <format>
#include <type_traits>

using PhysAddr  = std::uintptr_t;

using usize     = std::size_t;
using isize     = std::make_signed_t<usize>;

using u8        = std::uint8_t;
using u16       = std::uint16_t;
using u32       = std::uint32_t;
using u64       = std::uint64_t;

using i8        = std::int8_t;
using i16       = std::int16_t;
using i32       = std::int32_t;
using i64       = std::int64_t;

using symbol    = void*[];
using ErrorCode = std::errno_t;
using Error     = std::unexpected<ErrorCode>;

template <typename R>
using ErrorOr = std::expected<R, ErrorCode>;

inline constexpr u64 Bit(u64 n) { return (1ull << n); }

namespace BootInfo
{
    // We have to forward declare it here, to avoid recursive includes
    uintptr_t GetHHDMOffset();
}; // namespace BootInfo
