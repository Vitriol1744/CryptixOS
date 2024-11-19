/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Logger.hpp"

#include "Drivers/Serial.hpp"
#include "Drivers/Terminal.hpp"

#include <mutex>

namespace Logger
{
    static Terminal   terminal;
    static usize      logOutputs = 0;
    static std::mutex lock;

    void              InitializeTerminal()
    {
        terminal.Initialize(*BootInfo::GetFramebuffer());
        logOutputs |= LOG_OUTPUT_TERMINAL;
    }
    void SetLogOutput(usize output) { logOutputs = output; }

    void LogChar(u64 c)
    {
        usize len = 1;
        if (c == RESET_COLOR) len = 4;
        else if (c > 255) len = 5;
        LogString(reinterpret_cast<const char*>(&c), len);
        if (c == '\n') LogChar('\r');
    }

    void LogString(std::string_view string)
    {
        LogString(string, string.length());
    }

    void LogString(std::string_view str, usize len)
    {
        std::unique_lock guard(lock);
        if (logOutputs & LOG_OUTPUT_SERIAL)
            Serial::Write(std::string_view(str.data(), len));
        if (logOutputs & LOG_OUTPUT_TERMINAL)
            terminal.PrintString(str.data(), len);
    }
} // namespace Logger
