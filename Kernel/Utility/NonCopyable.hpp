/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

template <typename T>
class NonCopyable
{
  public:
    NonCopyable(const NonCopyable&) = delete;
    T& operator=(const T&)          = delete;

  protected:
    NonCopyable()          = default;
    virtual ~NonCopyable() = default;
};