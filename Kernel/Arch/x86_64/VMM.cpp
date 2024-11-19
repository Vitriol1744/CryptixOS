/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Memory/VirtualMemoryManager.hpp"

constexpr usize                  PTE_PRESENT    = BIT(0);
constexpr usize                  PTE_WRITE      = BIT(1);
constexpr usize                  PTE_USER_SUPER = BIT(2);
constexpr usize                  PTE_PWT        = BIT(3);
constexpr usize                  PTE_PCD        = BIT(4);
[[maybe_unused]] constexpr usize PTE_ACCESSED   = BIT(5);
constexpr usize                  PTE_LPAGE      = BIT(7);
constexpr usize                  PTE_PAT4K      = BIT(7);
constexpr usize                  PTE_GLOBAL     = BIT(8);
[[maybe_unused]] constexpr usize PTE_CUSTOM0    = BIT(9);
[[maybe_unused]] constexpr usize PTE_CUSTOM1    = BIT(10);
[[maybe_unused]] constexpr usize PTE_CUSTOM2    = BIT(11);
constexpr usize                  PTE_PATLG      = BIT(12);
constexpr usize                  PTE_NOEXEC     = BIT(63ull);

struct [[gnu::packed]] PageTable
{
    PageTableEntry entries[512];
};
static bool     gib1Pages            = false;

uintptr_t       Arch::pteAddressMask = 0x000FFFFFFFFFF000;
uintptr_t       Arch::newTableFlags  = PTE_PRESENT | PTE_WRITE | PTE_USER_SUPER;

void*           Arch::AllocatePageTable() { return new PageTable; }

bool            PageTableEntry::IsValid() { return GetFlag(PTE_PRESENT); }
bool            PageTableEntry::IsLarge() { return GetFlag(PTE_LPAGE); }

PageTableEntry* PageMap::Virt2Pte(PageTable* topLevel, VirtAddr virt,
                                  bool allocate, usize psize)
{
    usize pml5Entry = (virt >> 48) & 0x1ffull;
    usize pml4Entry = (virt >> 39) & 0x1ffull;
    usize pml3Entry = (virt >> 30) & 0x1ffull;
    usize pml2Entry = (virt >> 21) & 0x1ffull;
    usize pml1Entry = (virt >> 12) & 0x1ffull;

    if (!topLevel) return nullptr;

    PageTable* pml4
        = BootInfo::GetPagingMode() == LIMINE_PAGING_MODE_X86_64_5LVL
            ? static_cast<PageTable*>(
                GetNextLevel(topLevel->entries[pml5Entry], allocate))
            : topLevel;
    if (!pml4) return nullptr;

    PageTable* pml3 = static_cast<PageTable*>(
        GetNextLevel(pml4->entries[pml4Entry], allocate));
    if (!pml3) return nullptr;

    if (psize == llPageSize || (pml3->entries[pml3Entry].IsLarge()))
        return &pml3->entries[pml3Entry];

    PageTable* pml2 = static_cast<PageTable*>(GetNextLevel(
        pml3->entries[pml3Entry], allocate, virt, llPageSize, psize));
    if (!pml2) return nullptr;

    if (psize == lPageSize || (pml2->entries[pml2Entry].IsLarge()))
        return &pml2->entries[pml2Entry];

    PageTable* pml1 = static_cast<PageTable*>(GetNextLevel(
        pml2->entries[pml2Entry], allocate, virt, lPageSize, psize));

    if (!pml1) return nullptr;

    return &pml1->entries[pml1Entry];
}

static usize ToX86Flags(Attributes attribs, CachingMode caching)
{

    uintptr_t pteFlags = PTE_PRESENT;
    if (std::to_underlying(attribs & Attributes::eWrite)) pteFlags |= PTE_WRITE;

    if (!(attribs & Attributes::eExecutable)) pteFlags |= PTE_NOEXEC;
    if (std::to_underlying(attribs & Attributes::eUser))
        pteFlags |= PTE_USER_SUPER;
    if (std::to_underlying(attribs & Attributes::eGlobal))
        pteFlags |= PTE_GLOBAL;
    if (std::to_underlying(attribs & Attributes::eLPage)
        || std::to_underlying(attribs & Attributes::eLLPage))
        pteFlags |= PTE_LPAGE;

    bool largePages = pteFlags & PTE_LPAGE;
    u64  patbit     = (largePages ? PTE_PATLG : PTE_PAT4K);

    switch (caching)
    {
        case CachingMode::eUncacheableStrong: pteFlags |= PTE_PCD; break;
        case CachingMode::eWriteCombining: pteFlags |= PTE_PCD | PTE_PWT; break;
        case CachingMode::eWriteThrough: pteFlags |= patbit; break;
        case CachingMode::eWriteProtected: pteFlags |= patbit | PTE_PWT; break;
        case CachingMode::eWriteBack: pteFlags |= patbit | PTE_PCD; break;
        case CachingMode::eUncacheable:
            pteFlags |= patbit | PTE_PCD | PTE_PWT;
            break;
    }

    return pteFlags;
}

uintptr_t PageMap::Virt2Phys(VirtAddr virt, Attributes flags)
{
    std::unique_lock guard(lock);

    auto             psize    = GetPageSize(flags);
    PageTableEntry*  pmlEntry = Virt2Pte(topLevel, virt, false, psize);
    if (!pmlEntry || !pmlEntry->GetFlag(PTE_PRESENT)) return -1;

    return pmlEntry->GetAddress() + (virt % psize);
}

bool PageMap::MapNoLock(VirtAddr virt, PhysAddr phys, Attributes flags,
                        CachingMode caching)
{

    auto psize = GetPageSize(flags);
    if (psize == llPageSize && !gib1Pages)
    {
        flags &= ~Attributes::eLLPage;
        flags |= Attributes::eLPage;

        for (usize i = 0; i < gib1; i += mib2)
            if (!MapNoLock(virt + i, phys + i, flags, caching)) return false;
        return true;
    }

    PageTableEntry* pmlEntry = Virt2Pte(topLevel, virt, true, psize);
    if (!pmlEntry)
    {
        LogError("VMM: Could not get page map entry for address {:#X}", virt);
        return false;
    }

    pmlEntry->Clear();
    pmlEntry->SetAddress(phys);
    pmlEntry->SetFlags(ToX86Flags(flags, caching), true);
    return true;
}

bool PageMap::UnmapNoLock(VirtAddr virt, Attributes flags)
{
    auto unmapOne = [this](VirtAddr virt, usize psize)
    {
        PageTableEntry* pmlEntry = Virt2Pte(topLevel, virt, false, psize);
        if (!pmlEntry)
        {
            LogError("VMM: Could not get page map entry for address 0x{:X}",
                     virt);
            return false;
        }

        pmlEntry->Clear();
        __asm__ volatile("invlpg (%0);" ::"r"(virt) : "memory");
        return true;
    };

    auto psize = this->GetPageSize(flags);
    if (psize == this->llPageSize && gib1Pages == false)
    {
        flags &= ~Attributes::eLLPage;
        flags |= Attributes::eLPage;

        for (usize i = 0; i < gib1; i += mib2)
            if (!unmapOne(virt + i, mib2)) return false;
        return true;
    }

    return unmapOne(virt, psize);
}

bool PageMap::SetFlags(VirtAddr virt, Attributes flags, CachingMode cache)
{
    std::unique_lock guard(lock);

    auto             psize    = GetPageSize(flags);
    PageTableEntry*  pmlEntry = Virt2Pte(topLevel, virt, true, psize);
    if (!pmlEntry)
    {
        LogError("VMM: Could not get page map entry for address 0x{:X}", virt);
        return false;
    }

    auto realflags = ToX86Flags(flags, cache);
    auto addr      = pmlEntry->GetAddress();

    pmlEntry->Clear();
    pmlEntry->SetAddress(addr);
    pmlEntry->SetFlags(realflags, true);
    return true;
}

namespace VirtualMemoryManager
{
    void SaveCurrentPageMap(PageMap& out)
    {
        uintptr_t cr3 = 0;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3)::"memory");

        out = ToHigherHalfAddress<uintptr_t>(cr3);
    }

    void LoadPageMap(PageMap& pageMap, bool)
    {
        uintptr_t topLevel = FromHigherHalfAddress<uintptr_t>(
            reinterpret_cast<uintptr_t>(pageMap.GetTopLevel()));

        __asm__ volatile("mov %0, %%cr3" ::"r"(topLevel));
    }
}; // namespace VirtualMemoryManager

PageMap::PageMap()
    : topLevel(new PageTable)
{
    this->llPageSize = gib1;
    this->lPageSize  = mib2;
    this->pageSize   = kib4;

    if (!VMM::GetKernelPageMap())
    {
        for (usize i = 256; i < 512; i++)
            GetNextLevel(this->topLevel->entries[i], true);

        usize pat = (0x07ull << 56ull) | (0x06ull << 48ull) | (0x05ull << 40ull)
                  | (0x04ull << 32ull) | (0x01ull << 24ull)
                  | (0x00ull << 16ull);

        u32 edx = pat >> 32;
        u32 eax = static_cast<u32>(pat);
        __asm__ volatile("wrmsr" ::"a"(eax), "d"(edx), "c"(0x277) : "memory");
        return;
    }

    for (usize i = 256; i < 512; i++)
        this->topLevel->entries[i]
            = VMM::GetKernelPageMap()->topLevel->entries[i];
}

static void DestroyLevel(PageMap* pageMap, PageTable* pml, usize start,
                         usize end, usize level)
{
    if (level == 0 || !pml) return;

    for (usize i = start; i < end; i++)
    {
        auto next = static_cast<PageTable*>(
            pageMap->GetNextLevel(pml->entries[i], false));
        if (!next) continue;

        DestroyLevel(pageMap, next, 0, 512, level - 1);
    }
    delete pml;
}

std::pair<Attributes, CachingMode> Arch2Flags(uintptr_t flags, bool lpages)
{
    Attributes ret1;
    if (flags & PTE_PRESENT) ret1 |= Attributes::eRead;
    if (flags & PTE_WRITE) ret1 |= Attributes::eWrite;
    if (!(flags & PTE_NOEXEC)) ret1 |= Attributes::eExecutable;
    if (flags & PTE_USER_SUPER) ret1 |= Attributes::eUser;
    if (flags & PTE_GLOBAL) ret1 |= Attributes::eGlobal;

    usize       patbit = (lpages ? PTE_PATLG : PTE_PAT4K);
    CachingMode ret2;

    if ((flags & (patbit | PTE_PCD | PTE_PWT)) == (patbit | PTE_PCD | PTE_PWT))
        ret2 = CachingMode::eUncacheable;
    else if ((flags & (patbit | PTE_PCD)) == (patbit | PTE_PCD))
        ret2 = CachingMode::eWriteBack;
    else if ((flags & (patbit | PTE_PWT)) == (patbit | PTE_PWT))
        ret2 = CachingMode::eWriteProtected;
    else if ((flags & (patbit)) == (patbit)) ret2 = CachingMode::eWriteThrough;
    else if ((flags & (PTE_PCD | PTE_PWT)) == (PTE_PCD | PTE_PWT))
        ret2 = CachingMode::eWriteCombining;
    else if ((flags & (PTE_PCD)) == (PTE_PCD))
        ret2 = CachingMode::eUncacheableStrong;

    return {ret1, ret2};
}

namespace Arch
{
    void InitializeVMM() { gib1Pages = false; }

    void DestroyPageMap(PageMap* pageMap)
    {
        DestroyLevel(pageMap, pageMap->GetTopLevel(), 0, 256,
                     BootInfo::GetPagingMode() == 0 ? 4 : 5);
    }

    bool IsCanonical(uintptr_t addr)
    {
        if (BootInfo::GetPagingMode() == 1)
            return (addr <= 0x00ffffffffffffffULL)
                || (addr >= 0xff00000000000000ULL);

        return (addr <= 0x00007fffffffffffULL)
            || (addr >= 0xffff800000000000ULL);
    }
}; // namespace Arch
