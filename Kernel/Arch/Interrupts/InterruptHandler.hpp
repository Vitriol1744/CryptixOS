/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Arch/Interrupts/InterruptManager.hpp"

#include <functional>

using InterruptHandlerFunction = void (*)(struct CPUContext*);

class InterruptHandler
{
  public:
    virtual ~InterruptHandler() = default;

    bool IsUsed() { return handler; }
    bool IsReserved() { return reserved; }

    bool Reserve()
    {
        if (IsReserved()) return false;
        return reserved = true;
    }

    inline void SetHandler(InterruptHandlerFunction handler)
    {
        this->handler = handler;
    }

    inline void SetInterruptVector(u8 interruptVector)
    {
        this->interruptVector = interruptVector;
    }
    inline u8 GetInterruptVector() const
    {
        return interruptVector.has_value() ? interruptVector.value() : 0;
    }

    bool operator()(struct CPUContext* ctx) { return HandleInterrupt(ctx); }
    virtual bool OnEndOfInterrupt() { return false; }

    InterruptHandler() = default;

    virtual bool HandleInterrupt(CPUContext* ctx)
    {
        if (handler) handler(ctx);
        return true;
    }

    inline void Register()
    {
        InterruptManager::RegisterInterruptHandler(*this);
    }

  private:
    InterruptHandlerFunction handler         = nullptr;
    std::optional<u8>        interruptVector = 0;
    bool                     reserved        = false;
};
