/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Memory/VirtualMemoryManager.hpp"

#include <frozen/map.h>

struct [[gnu::packed]] ttbr
{
    PageTableEntry entries[512]{};
};

struct PageTable
{
    ttbr* ttbr0;
    ttbr* ttbr1;
};

namespace VirtualMemoryManager
{
    constexpr usize                  VALID     = (1 << 0);
    constexpr usize                  TABLE     = (1 << 1);
    [[maybe_unused]] constexpr usize BLOCK     = (0 << 1);
    constexpr usize                  PAGE      = BIT(1);

    constexpr usize                  USER      = BIT(6);

    [[maybe_unused]] constexpr usize RW        = (0 << 7);
    constexpr usize                  RO        = (1 << 7);

    constexpr usize                  ACCESS    = (1 << 10);
    constexpr usize                  NOTGLOBAL = (1 << 11);
    constexpr usize                  EXECNEVER = (1ul << 54);

    [[maybe_unused]] constexpr usize NONSHARE  = (0 << 8);
    constexpr usize                  OUTSHARE  = (0b10 << 8);
    constexpr usize                  INSHARE   = (0b11 << 8);

    constexpr usize                  WB        = (0b00 << 2) | INSHARE;
    constexpr usize                  NC        = (0b01 << 2) | OUTSHARE;
    constexpr usize                  WT        = (0b10 << 2) | OUTSHARE;

    static usize                     vaWidth   = 0;
    static usize                     psize     = 0;

    void                             SaveCurrentPageMap(PageMap& pageMap)
    {
        u64 value = 0;
        __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(value));
        pageMap.GetTopLevel()->ttbr0 = ToHigherHalfAddress<ttbr*>(value);
        __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(value));
        pageMap.GetTopLevel()->ttbr1 = ToHigherHalfAddress<ttbr*>(value);
    }
    void LoadPageMap(PageMap& pageMap, bool hh = false)
    {
        __asm__ volatile(
            "msr ttbr0_el1, %0" ::"r"(FromHigherHalfAddress<uintptr_t>(
                reinterpret_cast<uintptr_t>(pageMap.GetTopLevel()->ttbr0))));
        if (hh == true)
            __asm__ volatile("msr ttbr1_el1, %0" ::"r"(
                FromHigherHalfAddress<uintptr_t>(reinterpret_cast<uintptr_t>(
                    pageMap.GetTopLevel()->ttbr1))));
    }
}; // namespace VirtualMemoryManager

using namespace VirtualMemoryManager;
namespace Arch
{
    uintptr_t pteAddressMask = 0;
    uintptr_t newTableFlags  = VALID | TABLE;

    void*     AllocatePageTable() { return new ttbr; }
}; // namespace Arch

bool       PageTableEntry::IsValid() { return GetFlag(VALID); }
bool       PageTableEntry::IsLarge() { return !GetFlag(TABLE); }

static u64 Cache2Flags(CachingMode caching)
{
    switch (caching)
    {
        case CachingMode::eUncacheable:
        case CachingMode::eUncacheableStrong: return NC;
        case CachingMode::eWriteThrough: return WT;

        default: return WB;
    }

    std::unreachable();
}

PageTableEntry* PageMap::Virt2Pte(VirtAddr virt, bool allocate, u64 psize,
                                  bool checkll)
{
    usize pml5Entry = (virt & (0x1ffull << 48)) >> 48;
    usize pml4Entry = (virt & (0x1ffull << 39)) >> 39;
    usize pml3Entry = (virt & (0x1ffull << 30)) >> 30;
    usize pml2Entry = (virt & (0x1ffull << 21)) >> 21;
    usize pml1Entry = (virt & (0x1ffull << 12)) >> 12;

    ttbr* half = (virt & (1ull << 63ull)) ? topLevel->ttbr1 : topLevel->ttbr0;
    if (!half) return nullptr;

    ttbr* pml4 = static_cast<ttbr*>(
        (BootInfo::GetPagingMode() == 1)
            ? GetNextLevel(half->entries[pml5Entry], allocate)
            : half);
    if (!pml4) return nullptr;

    ttbr* pml3 = static_cast<ttbr*>(
        GetNextLevel(pml4->entries[pml4Entry], allocate, psize));
    if (!pml3) return nullptr;
    if (psize == llPageSize || (checkll && pml3->entries[pml3Entry].IsLarge()))
        return &pml3->entries[pml3Entry];

    ttbr* pml2 = static_cast<ttbr*>(GetNextLevel(
        pml3->entries[pml3Entry], allocate, virt, llPageSize, psize));
    if (!pml2) return nullptr;

    if (psize == lPageSize || (checkll && pml2->entries[pml2Entry].IsLarge()))
        return &pml2->entries[pml2Entry];

    ttbr* pml1 = static_cast<ttbr*>(GetNextLevel(
        pml2->entries[pml2Entry], allocate, virt, lPageSize, psize));
    if (!pml1) return nullptr;

    return &pml1->entries[pml1Entry];
}

uintptr_t PageMap::Virt2Phys(VirtAddr virt, Attributes flags)
{
    std::unique_lock guard(this->lock);

    auto             psize     = this->GetPageSize(flags);
    PageTableEntry*  pml_entry = this->Virt2Pte(virt, false, psize, true);
    if (pml_entry == nullptr || !pml_entry->GetFlag(VALID)) return -1;

    return pml_entry->GetAddress() + (virt % psize);
}
bool PageMap::MapNoLock(uintptr_t vaddr, uintptr_t paddr, Attributes flags,
                        CachingMode cache)
{
    PageTableEntry* pml_entry
        = this->Virt2Pte(vaddr, true, this->GetPageSize(flags), false);
    if (pml_entry == nullptr)
    {
        LogError("VMM: Could not get page map entry for address 0x{:X}", vaddr);
        return false;
    }

    auto realflags = Flags2Arch(flags) | Cache2Flags(cache);

    pml_entry->Clear();
    pml_entry->SetAddress(paddr);
    pml_entry->SetFlags(realflags, true);
    return true;
}

bool PageMap::UnmapNoLock(uintptr_t vaddr, Attributes flags)
{
    PageTableEntry* pml_entry
        = this->Virt2Pte(vaddr, false, this->GetPageSize(flags), true);
    if (pml_entry == nullptr)
    {
        LogError("VMM: Could not get page map entry for address 0x{:X}", vaddr);
        return false;
    }

    pml_entry->Clear();

    usize addr = (0ull << 48ull) | (vaddr >> 12ul);
    asm volatile(
        "dsb st; \n\t"
        "tlbi vale1, %0;\n\t"
        "dsb sy; isb" ::"r"(addr)
        : "memory");
    return true;
}

bool PageMap::SetFlagsNoLock(uintptr_t vaddr, Attributes flags,
                             CachingMode cache)
{
    PageTableEntry* pml_entry
        = this->Virt2Pte(vaddr, false, this->GetPageSize(flags), true);
    if (pml_entry == nullptr)
    {
        LogError("VMM: Could not get page map entry for address 0x{:X}", vaddr);
        return false;
    }

    auto realflags = Flags2Arch(flags) | Cache2Flags(cache);
    auto addr      = pml_entry->GetAddress();

    pml_entry->Clear();
    pml_entry->SetAddress(addr);
    pml_entry->SetFlags(realflags, true);
    return true;
}

PageMap::PageMap()
    : topLevel(new PageTable{new ttbr, nullptr})
{
    this->llPageSize = psize * 512 * 512;
    this->lPageSize  = psize * 512;
    this->pageSize   = psize;

    if (VMM::GetKernelPageMap() == nullptr) this->topLevel->ttbr1 = new ttbr;
    else this->topLevel->ttbr1 = VMM::GetKernelPageMap()->topLevel->ttbr1;
}

bool IsCanonical(uintptr_t addr)
{
    if (vaWidth == 52)
        return (addr <= 0x000FFFFFFFFFFFFFULL)
            || (addr >= 0xFFF0000000000000ULL);
    else if (vaWidth == 48)
        return (addr <= 0x0000FFFFFFFFFFFFULL)
            || (addr >= 0xFFFF000000000000ULL);

    Panic("VMM: Unknown VA width {}", vaWidth);
}

uintptr_t Flags2Arch(Attributes flags)
{
    uintptr_t ret = VALID | ACCESS;
    if (!(flags & Attributes::eWrite)) ret |= RO;
    if (!(flags & Attributes::eExecutable)) ret |= EXECNEVER;
    if (std::to_underlying(flags & Attributes::eUser)) ret |= USER;
    if (!(flags & Attributes::eGlobal)) ret |= NOTGLOBAL;
    if (!((std::to_underlying(flags & Attributes::eLPage))
          || std::to_underlying(flags & Attributes::eLLPage)))
        ret |= PAGE;

    return ret;
}

std::pair<Attributes, CachingMode> Arch2Flags(usize flags, bool lpages)
{
    Attributes ret1{};

    if (flags & VALID) ret1 |= Attributes::eRead;
    if (!(flags & RO)) ret1 |= Attributes::eWrite;
    if (!(flags & EXECNEVER)) ret1 |= Attributes::eExecutable;
    if (flags & USER) ret1 |= Attributes::eUser;
    if (!(flags & NOTGLOBAL)) ret1 |= Attributes::eGlobal;

    CachingMode ret2;
    if (flags & NC) ret2 = CachingMode::eUncacheableStrong;
    if (flags & WT) ret2 = CachingMode::eWriteThrough;
    if (flags & WB) ret2 = CachingMode::eWriteBack;

    return {ret1, ret2};
}

void DestroyPageMap(PageMap* pmap)
{
    // TODO
}

enum paranges : uint64_t
{
    bits32 = 0b0000,
    bits36 = 0b0001,
    bits40 = 0b0010,
    bits42 = 0b0011,
    bits44 = 0b0100,
    bits48 = 0b0101,
    bits52 = 0b0110 // when lpa or lpa2
};

union mmfr0
{
    struct [[gnu::packed]]
    {
        uint64_t parange   : 4; // physical address range
        uint64_t asidbits  : 4; // 0b0000: 8 bits, 0b0010: 16 bits
        uint64_t bigend    : 4; // 0b0001: mixed-endian support
        uint64_t snsmem    : 4; // 0b0001: secure memory support
        uint64_t bigendel0 : 4; // 0b0001: el0 mixed-endian support
        uint64_t tgran16   : 4; // 0b0000: no 16kb, 0b0001: 16kb, 0b0010: 16kb
                                // with pa52 lpa2
        uint64_t tgran64   : 4; // 0b0000: 64kb, 0b1111: no 64kb
        uint64_t tgran4 : 4; // 0b0000: 4kb, 0b0001: 4kb with pa52 lpa2, 0b1111
                             // no 4kb
        uint64_t tgran16_2 : 4; // 0b0001: no st2 16kb, 0b0010: st2 16kb,
                                // 0b0011: st2 16kb with pa52 lpa2
        uint64_t tgran64_2 : 4; // 0b0001: no st2 64kb, 0b0010: st2 64kb
        uint64_t tgran4_2  : 4; // 0b0001: no st2 4kb, 0b0010: st2 4kb, 0b0011:
                                // st2 4kb with pa52 lpa2
        uint64_t exs  : 4; // 0b0000: no, 0b0001: disable context-sync exception
        uint64_t rsv0 : 8; // reserved
        uint64_t fgt  : 4; // 0b0001: fine-frained traps
        uint64_t ecv  : 4; // 0b0001: enchanced counter virt
    };
    uint64_t raw;
};
static_assert(sizeof(mmfr0) == 8);

union mmfr2
{
    struct [[gnu::packed]]
    {
        uint64_t cnp     : 4;
        uint64_t uao     : 4;
        uint64_t lsm     : 4;
        uint64_t iesb    : 4;
        uint64_t varange : 4; // 0b0000: 48bit va, 0b0001: 52bit va with 64kb
        uint64_t ccidx   : 4;
        uint64_t nv      : 4;
        uint64_t st      : 4;
        uint64_t at      : 4;
        uint64_t ids     : 4;
        uint64_t fwb     : 4;
        uint64_t rsv0    : 4;
        uint64_t ttl     : 4;
        uint64_t bbm     : 4;
        uint64_t evt     : 4;
        uint64_t e0pd    : 4;
    };
    uint64_t raw;
};
static_assert(sizeof(mmfr2) == 8);

static constexpr size_t psize_4kib  = 0x1000;
static constexpr size_t psize_16kib = 0x4000;
static constexpr size_t psize_64kib = 0x10000;

void                    Arch::InitializeVMM()
{
    mmfr0    aa64mmfr0;
    mmfr2    aa64mmfr2;
    uint64_t tcr_el1 = 0;

    asm volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(aa64mmfr0.raw));
    asm volatile("mrs %0, id_aa64mmfr2_el1" : "=r"(aa64mmfr2.raw));
    asm volatile("mrs %0, tcr_el1" : "=r"(tcr_el1));

    bool feat_lpa = (aa64mmfr0.parange == paranges::bits52);
    bool feat_lva = (aa64mmfr2.varange == 0b0001);

    if (aa64mmfr0.tgran4 != 0b1111) psize = psize_4kib;
    else if (aa64mmfr0.tgran16 != 0b0000) psize = psize_16kib;
    else if (aa64mmfr0.tgran64 == 0b0000) psize = psize_64kib;
    else Panic("VMM: Unknown page size");

    if (BootInfo::GetPagingMode() == 1
        && (((tcr_el1 << 59) & 1) == 1 && feat_lpa == true
            && ((psize == psize_64kib && feat_lva == true)
                || (psize == psize_16kib && aa64mmfr0.tgran16 == 0b0010)
                || (psize == psize_4kib && aa64mmfr0.tgran4 == 0b0001))))
        vaWidth = 52;
    else vaWidth = 48;

    // 48-Bit
    // 0b0000000000000000111111111111111111111111111111111111000000000000 :
    // 0x0000FFFFFFFFF000 -> 4kib pages
    // 0b0000000000000000111111111111111111111111111111111100000000000000 :
    // 0x0000FFFFFFFFC000 -> 16kib pages
    // 0b0000000000000000111111111111111111111111111111110000000000000000 :
    // 0x0000FFFFFFFF0000 -> 64kib pages

    // 52-Bit
    // 0b0000000000000011111111111111111111111111111111111111000000000000 :
    // 0x0003FFFFFFFFF000 -> 4kib pages
    // 0b0000000000000011111111111111111111111111111111111100000000000000 :
    // 0x0003FFFFFFFFC000 -> 16kib pages
    // 0b0000000000000000111111111111111111111111111111110000000000000000 :
    // 0x0000FFFFFFFF0000 -> 64kib pages

    pteAddressMask = 0x0000FFFFFFFFF000;
}
