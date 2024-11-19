/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

enum class DeliveryMode
{
    eFixed       = 0b000,
    eLowPriority = 0b001,
    eSmi         = 0b010,
    eNmi         = 0b100,
    eInit        = 0b101,
    eExtInt      = 0b111,
};

enum class DestinationMode
{
    ePhysical = 0,
    eLogical  = 1,
};

enum class IoApicRedirectionFlags
{
    eActiveLow      = BIT(0),
    eLevelTriggered = BIT(1),
    eMasked         = BIT(2),
};

inline IoApicRedirectionFlags operator~(const IoApicRedirectionFlags& value)
{
    return static_cast<IoApicRedirectionFlags>(~static_cast<int>(value));
}
inline IoApicRedirectionFlags operator&(const IoApicRedirectionFlags& left,
                                        const IoApicRedirectionFlags& right)
{
    return static_cast<IoApicRedirectionFlags>(static_cast<int>(left)
                                               & static_cast<int>(right));
}
inline IoApicRedirectionFlags operator|(const IoApicRedirectionFlags& left,
                                        const IoApicRedirectionFlags& right)
{
    return static_cast<IoApicRedirectionFlags>(static_cast<i32>(left)
                                               | static_cast<i32>(right));
}
inline IoApicRedirectionFlags& operator|=(IoApicRedirectionFlags&       left,
                                          const IoApicRedirectionFlags& right)
{
    return left = left | right;
}

struct IoApicRedirectionEntry
{
    union
    {
        u8                     vector;
        DeliveryMode           deliveryMode    : 3;
        DestinationMode        destinationMode : 1;
        u8                     deliveryStatus  : 1;
        IoApicRedirectionFlags flags           : 4;
        u64                    reserved        : 39;
        u64                    destination     : 8;
    };
    union
    {
        u32 low;
        u32 high;
    };
};

class IoApic
{
  public:
    IoApic() = default;
    IoApic(uintptr_t baseAddress, u32 gsiBase);

    static void RedirectIRQ(u32 gsi, IoApicRedirectionEntry& entry);
    static void Mask(u32 gsi);
    static void Unmask(u32 gsi);

    static void MaskIRQ(u32 irq);
    static void UnmaskIRQ(u32 irq);

    static void Initialize();

  private:
    uintptr_t      baseAddress           = 0;
    u32            gsiBase               = 0;
    usize          redirectionEntryCount = 0;

    static IoApic* GetIoApicForGsi(u32 gsi);

    u32            Read(u32 reg);
    void           Write(u32 reg, u32 value);

    u64            GetRedirectionEntry(u32 index);
    void           SetRedirectionEntry(u32 index, u64 value);
    void SetRedirectionEntry(u32 index, const IoApicRedirectionEntry& entry);
};
