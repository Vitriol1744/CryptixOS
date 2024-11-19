/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Memory/VirtualMemoryManager.hpp"

#include <vector>

using pid_t = i64;
using tid_t = i64;

struct Process
{
    Process() = default;
    Process(std::string_view name)
        : name(name)
    {
    }

    pid_t                       pid;
    std::string                 name;
    PageMap*                    pageMap;

    std::atomic<tid_t>          nextTid;

    Process*                    parent;
    std::vector<struct Thread*> threads;
    std::vector<Process*>       children;
    std::vector<Process*>       zombies;
};
