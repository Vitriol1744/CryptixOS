/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

#include <uacpi/resources.h>
#include <uacpi/utilities.h>

#define CONCAT(a, b)       CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a##b
#define UNIQUE_NAME(name)  CONCAT(_##name##__, CONCAT(__COUNTER__, __LINE__))

struct [[gnu::packed, gnu::aligned(8)]] Module
{
    const char* Name;
    bool        Initialized;
    bool        Failed;

    bool        (*Initialize)();
    void        (*Terminate)();
};

bool LoadModule(Module* drv);

#define MODULE_SECTION      ".modules"
#define MODULE_DATA_SECTION ".modules.data"

#define MODULE_INIT(name, init)                                                \
    extern "C" [[gnu::section(MODULE_SECTION), gnu::used]] const Module        \
    CONCAT(__acpi_driver, UNIQUE_NAME(name))                                   \
        = {.Name        = #name,                                               \
           .Initialized = false,                                               \
           .Failed      = false,                                               \
           .Initialize  = init,                                                \
           .Terminate   = nullptr}

#define MODULE_EXIT(name, exit)                                                \
    extern "C" [[gnu::section(MODULE_SECTION "." #name)], gnu::used]] \
    void (*Terminate)() = exit;
