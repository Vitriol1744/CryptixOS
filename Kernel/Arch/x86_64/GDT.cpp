/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "GDT.hpp"

CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_CODE_READABLE
    = BIT(1);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_DATA_WRITABLE
    = BIT(1);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_DATA_GROWS_DOWN
    = BIT(2);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_CODE_RING_LESSEQ
    = BIT(2);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_CODE_SEGMENT
    = BIT(3);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_CODE_OR_DATA
    = BIT(4);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_RING1 = BIT(5);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_RING2 = BIT(6);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_RING3
    = BIT(5) | BIT(6);
CTOS_UNUSED inline static constexpr const usize GDT_ACCESS_PRESENT = BIT(7);

CTOS_UNUSED inline static constexpr const usize GDT_FLAG_64BIT_DESCRIPTOR
    = BIT(1);
CTOS_UNUSED inline static constexpr const usize GDT_FLAG_32BIT_DESCRIPTOR
    = BIT(2);
CTOS_UNUSED inline static constexpr const usize GDT_FLAG_GRANULARITY_4K
    = BIT(3);

CTOS_UNUSED inline static constexpr const usize TSS_FLAG_PRESENT = BIT(3);
CTOS_UNUSED inline static constexpr const usize TSS_FLAG_INACTIVE
    = BIT(3) | BIT(0);

CTOS_UNUSED inline static constexpr const usize TSS_SELECTOR = 0x28;

struct SegmentDescriptor
{
    u16 limitLow;
    u16 baseLow;
    u8  baseMiddle;
    u8  access;
    u8  limitHigh : 4;
    u8  flags     : 4;
    u8  baseHigh;
} __attribute__((packed));
struct TaskStateSegmentDescriptor
{
    u16 length;
    u16 baseLow;
    u8  baseMiddle1;
    u8  flags1;
    u8  flags2;
    u8  baseMiddle2;
    u32 baseHigh;
    u32 reserved;
} __attribute__((packed));
struct GDTEntries
{
    SegmentDescriptor          null;
    SegmentDescriptor          kernelCode;
    SegmentDescriptor          kernelData;
    SegmentDescriptor          userCode;
    SegmentDescriptor          userData;
    TaskStateSegmentDescriptor tss;
} __attribute__((packed)) gdtEntries;

#define GDTWriteEntry(_entry, _base, _limit, _access, _flags)                  \
    {                                                                          \
        (_entry)->limitLow   = _limit & 0xffff;                                \
        (_entry)->baseLow    = _base & 0xffff;                                 \
        (_entry)->baseMiddle = (_base >> 16) & 0xff;                           \
        (_entry)->access     = _access | GDT_ACCESS_PRESENT;                   \
        (_entry)->limitHigh  = (_limit >> 16) & 0xf;                           \
        (_entry)->flags      = _flags;                                         \
        (_entry)->baseHigh   = (_base >> 24) & 0xff;                           \
    }
#define TSSWriteEntry(_entry, _base)                                           \
    {                                                                          \
        (_entry)->length      = sizeof(TaskStateSegment);                      \
        (_entry)->baseLow     = (u16)(_base & 0xffff);                         \
        (_entry)->baseMiddle1 = (u8)((_base >> 16) & 0xff);                    \
        (_entry)->flags1      = ((1 << 3) << 4) | (1 << 3 | 1 << 0);           \
        (_entry)->flags2      = 0;                                             \
        (_entry)->baseMiddle2 = (u8)((_base >> 24) & 0xff);                    \
        (_entry)->baseHigh    = (u32)(_base >> 32);                            \
        (_entry)->reserved    = 0;                                             \
    }

namespace GDT
{
    void Initialize(u64 id)
    {
        LogTrace("Loading GDT...");
        memset(&gdtEntries.null, 0, sizeof(SegmentDescriptor));

        u8 userCodeAccess = GDT_ACCESS_CODE_READABLE | GDT_ACCESS_CODE_SEGMENT
                          | GDT_ACCESS_CODE_OR_DATA | GDT_ACCESS_RING3;
        u8 userDataAccess = GDT_ACCESS_DATA_WRITABLE | GDT_ACCESS_CODE_OR_DATA
                          | GDT_ACCESS_RING3;

        GDTWriteEntry(&gdtEntries.kernelCode, 0, 0xffffffff,
                      GDT_ACCESS_CODE_OR_DATA | GDT_ACCESS_CODE_SEGMENT
                          | GDT_ACCESS_CODE_READABLE,
                      0xa);

        GDTWriteEntry(&gdtEntries.kernelData, 0, 0xffffffff,
                      GDT_ACCESS_CODE_OR_DATA | GDT_ACCESS_DATA_WRITABLE, 0xa);
        GDTWriteEntry(&gdtEntries.userCode, 0, 0, userCodeAccess, 0xa);
        GDTWriteEntry(&gdtEntries.userData, 0, 0, userDataAccess, 0xa);
        TSSWriteEntry(&gdtEntries.tss, 0ull);

        Load();
        LogInfo("GDT: Initialized on CPU[{}]", id);
    }

    void Load()
    {
        struct
        {
            u16       limit;
            uintptr_t base;
        } __attribute__((packed)) gdtr;
        gdtr.limit = sizeof(gdtEntries) - 1;
        gdtr.base  = reinterpret_cast<uintptr_t>(&gdtEntries);

        __asm__ volatile(
            "lgdt %0\n"
            "mov %%rsp, %%rbx\n"
            "push %1\n"
            "push %%rbx\n"
            "pushf\n"
            "push %2\n"
            "push $1f\n"
            "iretq\n"
            "1:\n"
            "mov %1, %%ds\n"
            "mov %1, %%es\n"
            "mov %1, %%fs\n"
            "mov %1, %%gs\n"
            "mov %1, %%ss"
            :
            : "m"(gdtr), "r"(KERNEL_DATA_SELECTOR), "r"(KERNEL_CODE_SELECTOR)
            : "rbx", "memory");

        __asm__ volatile("ltr %0" ::"r"(static_cast<uint16_t>(TSS_SELECTOR)));
    }
} // namespace GDT
