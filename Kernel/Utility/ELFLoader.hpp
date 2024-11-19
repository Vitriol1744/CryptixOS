/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Memory/VirtualMemoryManager.hpp"
#include "Utility/Types.hpp"

#include <elf.h>
#include <limine.h>
#include <optional>
#include <string>
#include <utility>

struct AuxVal
{
    u64 atentry;
    u64 atphdr;
    u64 atphent;
    u64 atphnum;
};

namespace ELFLoader
{
    std::optional<std::pair<AuxVal, std::string>> Load(limine_file* file,
                                                       PageMap*     pagemap,
                                                       uintptr_t    base,
                                                       uintptr_t*, uintptr_t*);
};
