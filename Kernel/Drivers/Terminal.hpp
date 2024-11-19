/*
 * Created by v1tr10l7 on 23.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/BootInfo.hpp"

constexpr const u32 TERMINAL_COLOR_BLACK = 0x000000;

class Terminal      final
{
  public:
    Terminal() = default;
    bool        Initialize(Framebuffer& framebuffer);

    void        Clear(u32 color = TERMINAL_COLOR_BLACK);
    void        PutChar(u64 c);
    void        PrintString(const char* string, usize length);
    void        PrintString(const char* string);
    void        ScrollDown(u8 lines = 1);

    inline u64  GetColor() const { return (0x00d4b400383c3c); }
    inline u32  GetForegroundColor() const { return foregroundColor; }
    inline u32  GetBackgroundColor() const { return backgroundColor; }

    inline void SetColor(u64 color)
    {
        SetForegroundColor(color >> 32);
        SetBackgroundColor(color);
    }
    inline void SetForegroundColor(u32 color) { foregroundColor = color; }
    inline void SetBackgroundColor(u32 color) { backgroundColor = color; }

  private:
    bool        initialized     = false;
    Framebuffer framebuffer     = {};
    u32         x               = 0;
    u32         y               = 0;
    u32         foregroundColor = 0x00ffff;
    u32         backgroundColor = 0x000000;

    void        PutPixel(u32 pixel, u32 x, u32 y);
    usize       UpdateColors(const char* character);
};
