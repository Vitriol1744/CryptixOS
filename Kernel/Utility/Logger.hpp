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

constexpr usize            LOG_OUTPUT_SERIAL        = 1;
constexpr usize            LOG_OUTPUT_TERMINAL      = 2;

static constexpr const u64 FOREGROUND_COLOR_BLACK   = 0x6d30335b1b;
static constexpr const u64 FOREGROUND_COLOR_RED     = 0x6d31335b1b;
static constexpr const u64 FOREGROUND_COLOR_GREEN   = 0x6d32335b1b;
static constexpr const u64 FOREGROUND_COLOR_YELLOW  = 0x6d33335b1b;
static constexpr const u64 FOREGROUND_COLOR_BLUE    = 0x6d34335b1b;
static constexpr const u64 FOREGROUND_COLOR_MAGENTA = 0x6d35335b1b;
static constexpr const u64 FOREGROUND_COLOR_CYAN    = 0x6d36335b1b;
static constexpr const u64 FOREGROUND_COLOR_WHITE   = 0x6d37335b1b;

static constexpr const u64 BACKGROUND_COLOR_BLACK   = 0x6d30345b1b;
static constexpr const u64 BACKGROUND_COLOR_RED     = 0x6d31345b1b;
static constexpr const u64 BACKGROUND_COLOR_GREEN   = 0x6d32345b1b;
static constexpr const u64 BACKGROUND_COLOR_YELLOW  = 0x6d33345b1b;
static constexpr const u64 BACKGROUND_COLOR_BLUE    = 0x6d34345b1b;
static constexpr const u64 BACKGROUND_COLOR_MAGENTA = 0x6d35345b1b;
static constexpr const u64 BACKGROUND_COLOR_CYAN    = 0x6d36345b1b;
static constexpr const u64 BACKGROUND_COLOR_WHITE   = 0x6d37345b1b;

static constexpr const u64 RESET_COLOR              = 0x6d305b1b;

template <typename T>
char* toString(T value, char* str, int base)
{
    T    i          = 0;
    bool isNegative = false;

    if (value == 0)
    {
        str[i++] = '0';
        str[i]   = 0;
        return str;
    }

    if (value < 0 && base == 10)
    {
        isNegative = true;
        value      = -value;
    }

    while (value != 0)
    {
        T reminder = value % base;
        str[i++]   = static_cast<char>(reminder > 9 ? reminder - 10 + 'a'
                                                    : reminder + '0');
        value /= base;
    }

    if (isNegative) str[i++] = '-';
    str[i]  = 0;

    T start = 0;
    T end   = i - 1;
    while (start < end)
    {
        char c         = *(str + start);
        *(str + start) = *(str + end);
        *(str + end)   = c;
        start++;
        end--;
    }

    return str;
}

namespace Logger
{
    void        InitializeTerminal();
    void        SetLogOutput(usize output);

    void        LogChar(u64 c);
    void        LogString(std::string_view string);
    void        LogString(std::string_view, usize len);

    inline void Log(LogLevel logLevel, std::string_view string)
    {
        if (logLevel != LogLevel::eNone) LogChar('[');
        switch (logLevel)
        {
            case LogLevel::eDebug:
                LogChar(FOREGROUND_COLOR_MAGENTA);
                LogString("Debug");
                break;
            case LogLevel::eTrace:
                LogChar(FOREGROUND_COLOR_GREEN);
                LogString("Trace");
                break;
            case LogLevel::eInfo:
                LogChar(FOREGROUND_COLOR_CYAN);
                LogString("Info");
                break;
            case LogLevel::eWarn:
                LogChar(FOREGROUND_COLOR_YELLOW);
                LogString("Warn");
                break;
            case LogLevel::eError:
                LogChar(FOREGROUND_COLOR_RED);
                LogString("Error");
                break;
            case LogLevel::eFatal:
                LogChar(BACKGROUND_COLOR_RED);
                LogChar(FOREGROUND_COLOR_WHITE);
                LogString("Fatal");
                break;

            default: break;
        }

        if (logLevel != LogLevel::eNone)
        {
            LogChar(FOREGROUND_COLOR_WHITE);
            LogChar(BACKGROUND_COLOR_BLACK);
            LogChar(RESET_COLOR);
            LogString("]: ");
        }
        LogString(string);
        LogChar('\n');
    }
} // namespace Logger

#if !NDEBUG
    #define LogDebug(...)                                                      \
        Logger::Log(LogLevel::eDebug, std::format(__VA_ARGS__))
#else
    #define LogDebug(...)
#endif

#define LogMessage(...) Logger::Log(LogLevel::eNone, std::format(__VA_ARGS__))

#define ENABLE_LOGGING  true
#if ENABLE_LOGGING == true
    #define LogTrace(...)                                                      \
        Logger::Log(LogLevel::eTrace, std::format(__VA_ARGS__))
    #define LogInfo(...) Logger::Log(LogLevel::eInfo, std::format(__VA_ARGS__))
    #define LogWarn(...) Logger::Log(LogLevel::eWarn, std::format(__VA_ARGS__))
    #define LogError(...)                                                      \
        Logger::Log(LogLevel::eError, std::format(__VA_ARGS__))
    #define LogFatal(...)                                                      \
        Logger::Log(LogLevel::eFatal, std::format(__VA_ARGS__))
#else
    #define LogTrace(...)
    #define LogInfo(...)
    #define LogWarn(...)
    #define LogError(...)
    #define LogFatal(...)
#endif
