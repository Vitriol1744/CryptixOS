/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

namespace Syscall::Process
{
    pid_t SysFork(Syscall::Arguments& args);
    int   SysExecve(Syscall::Arguments& args);
    int   SysExit(Syscall::Arguments& args);
}; // namespace Syscall::Process
