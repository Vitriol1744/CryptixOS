/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "PhysicalMemoryManager.hpp"

#include "Common.hpp"

#include "Memory/KernelHeap.hpp"
#include "Utility/Bitmap.hpp"
#include "Utility/BootInfo.hpp"
#include "Utility/Math.hpp"

#include <mutex>

namespace PhysicalMemoryManager
{
    namespace
    {
        Bitmap     bitmap{};
        uintptr_t  memoryTop   = 0;
        // uintptr_t  usableMemoryTop = 0;
        usize      totalMemory = 0;
        usize      usedMemory  = 0;
        std::mutex lock;

        void*      FindFreeRegion(usize& start, usize count, usize limit)
        {
            usize contiguousPages = 0;
            while (start < limit)
            {
                if (!bitmap.GetIndex(start++))
                {
                    if (++contiguousPages == count)
                    {
                        usize page = start - count;
                        for (usize i = page; i < start; i++)
                            bitmap.SetIndex(i, true);
                        return reinterpret_cast<void*>(page * PAGE_SIZE);
                    }
                }
                else contiguousPages = 0;
            }

            return nullptr;
        }
    } // namespace

    bool Initialize()
    {
        usize     entryCount = 0;
        MemoryMap memoryMap  = BootInfo::GetMemoryMap(entryCount);
        if (entryCount == 0) return false;

        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* currentEntry = memoryMap[i];
            uintptr_t       top = currentEntry->base + currentEntry->length;
            memoryTop           = std::max(memoryTop, top);
            switch (currentEntry->type)
            {
                case MEMORY_MAP_USABLE:
                    totalMemory += currentEntry->length;
                    memoryTop = std::max(memoryTop, top);
                    break;
                case MEMORY_MAP_ACPI_RECLAIMABLE:
                case MEMORY_MAP_BOOTLOADER_RECLAIMABLE:
                case MEMORY_MAP_KERNEL_AND_MODULES:
                    totalMemory += currentEntry->length;
                    usedMemory += currentEntry->length;
                    break;
                default: break;
            }
        }

        if (memoryTop == 0) return false;

        usize bitmapSize = Math::AlignUp(memoryTop / PAGE_SIZE / 8, PAGE_SIZE);
        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* currentEntry = memoryMap[i];
            if (currentEntry->type != MEMORY_MAP_USABLE
                || currentEntry->length <= bitmapSize)
                continue;
            bitmap.data = reinterpret_cast<u8*>(currentEntry->base
                                                + BootInfo::GetHHDMOffset());
            bitmap.size = bitmapSize;
            bitmap.SetAll(0xff);
            currentEntry->base += bitmapSize;
            currentEntry->length -= bitmapSize;
            break;
        }

        for (size_t i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* currentEntry = memoryMap[i];
            if (currentEntry->type != MEMORY_MAP_USABLE) continue;

            for (uintptr_t page = 0; page < currentEntry->length;
                 page += PAGE_SIZE)
                bitmap.SetIndex((currentEntry->base + page) / PAGE_SIZE, false);
        }

        KernelHeap::Initialize();
        memoryTop = Math::AlignUp(memoryTop, 0x40000000);

        LogInfo(
            "Memory Map entries count: {}, Total Memory: {}, Free Memory: {}",
            entryCount, totalMemory, GetFreeMemory());
        return true;
    }

    void* AllocatePages(usize count)
    {
        std::unique_lock guard(lock);
        if (count == 0) return nullptr;
        static usize lastIndex = 0;

        usize        i         = lastIndex;
        void* ret = FindFreeRegion(lastIndex, count, memoryTop / PAGE_SIZE);
        if (!ret)
        {
            lastIndex = 0;
            ret       = FindFreeRegion(lastIndex, count, i);
            if (!ret) Panic("Out of memory!");
        }

        usedMemory += count * PAGE_SIZE;
        return ret;
    }
    void* CallocatePages(usize count)
    {
        void* ret = AllocatePages(count);
        if (!ret) return nullptr;

        uintptr_t higherHalfAddress
            = reinterpret_cast<uintptr_t>(ret) + BootInfo::GetHHDMOffset();
        memset(reinterpret_cast<void*>(higherHalfAddress), 0,
               PAGE_SIZE * count);

        return ret;
    }

    void FreePages(void* ptr, usize count)
    {
        std::unique_lock guard(lock);
        if (count == 0 || !ptr) return;
        uintptr_t page = reinterpret_cast<uintptr_t>(ptr) / PAGE_SIZE;

        for (uintptr_t i = page; i < page + count; i++)
            bitmap.SetIndex(i, false);
        usedMemory -= count * PAGE_SIZE;
    }

    uintptr_t GetMemoryTop() { return memoryTop; }
    uint64_t  GetTotalMemory() { return totalMemory; }
    uint64_t  GetFreeMemory() { return totalMemory - usedMemory; }
    uint64_t  GetUsedMemory() { return usedMemory; }
} // namespace PhysicalMemoryManager
