/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/BootInfo.hpp"
#include "Utility/Types.hpp"

#include <format>
#include <string>
#include <string_view>

enum class LogLevel
{
    eNone,
    eDebug,
    eTrace,
    eInfo,
    eWarn,
    eError,
    eFatal,
};

constexpr usize LOG_OUTPUT_E9            = Bit(0);
constexpr usize LOG_OUTPUT_SERIAL        = Bit(1);
constexpr usize LOG_OUTPUT_TERMINAL      = Bit(2);

constexpr u64   FOREGROUND_COLOR_BLACK   = 0x6d30335b1b;
constexpr u64   FOREGROUND_COLOR_RED     = 0x6d31335b1b;
constexpr u64   FOREGROUND_COLOR_GREEN   = 0x6d32335b1b;
constexpr u64   FOREGROUND_COLOR_YELLOW  = 0x6d33335b1b;
constexpr u64   FOREGROUND_COLOR_BLUE    = 0x6d34335b1b;
constexpr u64   FOREGROUND_COLOR_MAGENTA = 0x6d35335b1b;
constexpr u64   FOREGROUND_COLOR_CYAN    = 0x6d36335b1b;
constexpr u64   FOREGROUND_COLOR_WHITE   = 0x6d37335b1b;

constexpr u64   BACKGROUND_COLOR_BLACK   = 0x6d30345b1b;
constexpr u64   BACKGROUND_COLOR_RED     = 0x6d31345b1b;
constexpr u64   BACKGROUND_COLOR_GREEN   = 0x6d32345b1b;
constexpr u64   BACKGROUND_COLOR_YELLOW  = 0x6d33345b1b;
constexpr u64   BACKGROUND_COLOR_BLUE    = 0x6d34345b1b;
constexpr u64   BACKGROUND_COLOR_MAGENTA = 0x6d35345b1b;
constexpr u64   BACKGROUND_COLOR_CYAN    = 0x6d36345b1b;
constexpr u64   BACKGROUND_COLOR_WHITE   = 0x6d37345b1b;

constexpr u64   RESET_COLOR              = 0x6d305b1b;

#define CTOS_NO_KASAN __attribute__((no_sanitize("address")))

namespace Logger
{
    CTOS_NO_KASAN void EnableOutput(usize output);
    CTOS_NO_KASAN void DisableOutput(usize output);

    CTOS_NO_KASAN void LogChar(u64 c);
    CTOS_NO_KASAN void LogString(const char* string);

    CTOS_NO_KASAN void Log(LogLevel logLevel, const char*);
    CTOS_NO_KASAN void Logf(LogLevel logLevel, const char* format, ...);
    CTOS_NO_KASAN void Logv(LogLevel logLevel, const char* format,
                            va_list& args);
} // namespace Logger

#ifdef CTOS_BUILD_DEBUG
    #define LogDebug(...)                                                      \
        Logger::Log(LogLevel::eDebug, std::format(__VA_ARGS__))
#else
    #define LogDebug(...)
#endif

#define LogMessage(...)                                                        \
    Logger::Log(LogLevel::eNone, std::format(__VA_ARGS__).data())

#define ENABLE_LOGGING true
#if ENABLE_LOGGING == true
    #define EarlyLogTrace(...) Logger::Logf(LogLevel::eTrace, __VA_ARGS__)
    #define EarlyLogInfo(...)  Logger::Logf(LogLevel::eInfo, __VA_ARGS__)
    #define EarlyLogWarn(...)  Logger::Logf(LogLevel::eWarn, __VA_ARGS__)
    #define EarlyLogError(...) Logger::Logf(LogLevel::eError, __VA_ARGS__)
    #define EarlyLogFatal(...) Logger::Logf(LogLevel::eFatal, __VA_ARGS__)

    #define LogTrace(...)                                                      \
        Logger::Log(LogLevel::eTrace, std::format(__VA_ARGS__).data())
    #define LogInfo(...)                                                       \
        Logger::Log(LogLevel::eInfo, std::format(__VA_ARGS__).data())
    #define LogWarn(...)                                                       \
        Logger::Log(LogLevel::eWarn, std::format(__VA_ARGS__).data())
    #define LogError(...)                                                      \
        Logger::Log(LogLevel::eError, std::format(__VA_ARGS__).data())
    #define LogFatal(...)                                                      \
        Logger::Log(LogLevel::eFatal, std::format(__VA_ARGS__).data())
#else
    #define EarlyLogTrace(...)
    #define EarlyLogInfo(...)
    #define EarlyLogWarn(...)
    #define EarlyLogError(...)
    #define EarlyLogFatal(...)

    #define LogTrace(...)
    #define LogInfo(...)
    #define LogWarn(...)
    #define LogError(...)
    #define LogFatal(...)
#endif
