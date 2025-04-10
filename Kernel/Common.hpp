/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/Arch.hpp>

#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Library/Stacktrace.hpp>
#include <Prism/Core/Types.hpp>

#include <format>
#include <string_view>

inline constexpr u64 BIT(u64 n) { return (1ull << n); }

#define CTOS_UNUSED               [[maybe_unused]]
#define CtosUnused(var)           ((void)var)
#define CTOS_GET_FRAME_ADDRESS(n) __builtin_frame_address(n)

#define CTOS_ARCH_X86_64          0
#define CTOS_ARCH_AARCH64         1
#define CTOS_ARCH_RISC_V          2

#define CTOS_DEBUG_SYSCALLS       0
#if CTOS_DEBUG_SYSCALLS == 1
    #define DebugSyscall(...) LogDebug(__VA_ARGS__)
#else
    #define DebugSyscall(...)
#endif

constexpr usize operator""_kib(unsigned long long count)
{
    return count * 1024;
}
constexpr usize operator""_mib(unsigned long long count)
{
    return count * 1024_kib;
}
constexpr usize operator""_gib(unsigned long long count)
{
    return count * 1024_mib;
}
