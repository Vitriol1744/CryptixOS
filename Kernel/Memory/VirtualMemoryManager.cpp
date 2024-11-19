/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "VirtualMemoryManager.hpp"

#include "Utility/BootInfo.hpp"
#include "Utility/Math.hpp"

#include <cerrno>

extern "C" symbol text_start_addr;
extern "C" symbol text_end_addr;
extern "C" symbol rodata_start_addr;
extern "C" symbol rodata_end_addr;
extern "C" symbol data_start_addr;
extern "C" symbol data_end_addr;

static PageMap*   kernelPageMap;
static uintptr_t  vspbaddrs[6]{};

void* PageMap::GetNextLevel(PageTableEntry& entry, bool allocate, VirtAddr virt,
                            usize opsize, usize psize)
{
    void* ret = nullptr;

    if (!entry.IsValid())
    {
        if (!allocate) return nullptr;

        ret = Arch::AllocatePageTable();
        entry.SetAddress(
            FromHigherHalfAddress<uintptr_t>(reinterpret_cast<uintptr_t>(ret)));
        entry.SetFlags(Arch::newTableFlags, true);

        return ret;
    }

    if (entry.IsLarge() && opsize != usize(-1))
    {
        auto [oldFlags, oldCaching]
            = Arch2Flags(entry.GetFlags(), opsize > this->pageSize);
        auto oldPhys = entry.GetAddress();
        auto oldVirt = virt & ~(opsize - 1);

        if (oldPhys & (opsize - 1))
            Panic("VMM: Unexpected page table entry address: 0x{:X}", oldPhys);

        ret = Arch::AllocatePageTable();
        entry.SetAddress(
            FromHigherHalfAddress<uintptr_t>(reinterpret_cast<uintptr_t>(ret)));
        entry.SetFlags(Arch::newTableFlags, true);

        for (size_t i = 0; i < opsize; i += psize)
            this->MapNoLock(oldVirt + i, oldPhys + i,
                            oldFlags | GetPageSizeFlags(psize), oldCaching);
    }

    return ToHigherHalfAddress<void*>(
        static_cast<uintptr_t>(entry.GetAddress()));
}

namespace VirtualMemoryManager
{
    static bool s_Initialized = false;

    void        Initialize()
    {
        if (s_Initialized) return;
        s_Initialized = true;
        LogTrace("VMM: Initializing...");
        Arch::InitializeVMM();

        kernelPageMap = new PageMap();
        Assert(kernelPageMap->GetTopLevel() != 0);

        auto [psize, flags] = kernelPageMap->RequiredSize(gib1 * 4);
        for (uintptr_t i = 0; i < gib1 * 4; i += psize)
            Assert(kernelPageMap->Map(ToHigherHalfAddress<uintptr_t>(i), i,
                                      Attributes::eRW | flags));

        usize entryCount = 0;
        auto  entries    = BootInfo::GetMemoryMap(entryCount);
        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* mmap = entries[i];

            uintptr_t       base
                = Math::AlignDown(mmap->base, kernelPageMap->GetPageSize());
            uintptr_t top = Math::AlignUp(mmap->base + mmap->length,
                                          kernelPageMap->GetPageSize());
            if (top < gib1 * 4) continue;

            CachingMode cache = CachingMode::eWriteBack;
            if (mmap->type == MEMORY_MAP_FRAMEBUFFER)
                cache = CachingMode::eWriteCombining;

            auto size           = top - base;
            auto [psize, flags] = kernelPageMap->RequiredSize(size);

            auto alsize         = Math::AlignDown(size, psize);
            auto diff           = size - alsize;

            for (uintptr_t t = base; t < (base + alsize); t += psize)
            {
                if (t < gib1 * 4) continue;

                Assert(kernelPageMap->Map(ToHigherHalfAddress<uintptr_t>(t), t,
                                          Attributes::eRW | flags, cache));
            }
            base += alsize;

            for (uintptr_t t = base; t < (base + diff);
                 t += kernelPageMap->GetPageSize())
            {
                if (t < gib1 * 4) continue;

                Assert(kernelPageMap->Map(ToHigherHalfAddress<uintptr_t>(t), t,
                                          Attributes::eRW, cache));
            }
        }

        for (usize i = 0; i < BootInfo::GetKernelFile()->size;
             i += kernelPageMap->GetPageSize())
        {
            PhysAddr paddr = BootInfo::GetKernelPhysicalAddress() + i;
            VirtAddr vaddr = BootInfo::GetKernelVirtualAddress() + i;
            Assert(kernelPageMap->Map(vaddr, paddr, Attributes::eRWX,
                                      CachingMode::eWriteBack));
        }

        {
            auto base = ToHigherHalfAddress<uintptr_t>(
                Math::AlignUp(PMM::GetMemoryTop(), gib1));
            vspbaddrs[0] = base;

            for (size_t i = 1; auto& entry : vspbaddrs)
                entry = base + (gib1 * i);
        }

        kernelPageMap->Load();
        LogInfo("VMM: Loaded kernel page map");
    }

    uintptr_t AllocateSpace(vsptypes type, usize increment, usize alignment,
                            bool lowerHalf)
    {
        Assert(alignment <= PMM::PAGE_SIZE);

        auto       index = std::to_underlying(type);

        uintptr_t* entry = &vspbaddrs[index];
        if (type != vsptypes::other && increment > 0
            && *entry + increment > (vspbaddrs[0] + (gib1 * (index + 1))))
            entry = &vspbaddrs[std::to_underlying(vsptypes::other)];

        uintptr_t ret = alignment ? Math::AlignUp(*entry, alignment) : *entry;
        *entry += increment + (ret - *entry);

        return lowerHalf ? FromHigherHalfAddress<uintptr_t>(ret) : ret;
    }

    PageMap* GetKernelPageMap()
    {
        if (!s_Initialized) Initialize();
        return kernelPageMap;
    }

} // namespace VirtualMemoryManager
