/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Memory/PhysicalMemoryManager.hpp"
#include "Utility/BootInfo.hpp"

#include <mutex>

#define HIGHER_HALF_OFFSET BootInfo::GetHHDMOffset()
inline static constexpr const usize PTE_ADDR_MASK = 0x000ffffffffff000;
inline static constexpr usize       PTE_GET_ADDR(usize value)
{
    return value & PTE_ADDR_MASK;
}

inline constexpr size_t kib4 = 0x1000;
inline constexpr size_t mib2 = 0x200000;
inline constexpr size_t gib1 = 0x40000000;

using PhysAddr               = uintptr_t;
using VirtAddr               = uintptr_t;

enum class Attributes
{
    eRead       = BIT(0),
    eWrite      = BIT(1),
    eExecutable = BIT(2),
    eUser       = BIT(3),
    eGlobal     = BIT(4),
    eLPage      = BIT(5),
    eLLPage     = BIT(6),

    eRW         = eRead | eWrite,
    eRWX        = eRW | eExecutable,
    eRWXU       = eRWX | eUser
};
inline bool operator!(const Attributes& value)
{
    return !static_cast<usize>(value);
}
inline Attributes operator~(const Attributes& value)
{
    return static_cast<Attributes>(~static_cast<int>(value));
}

inline Attributes operator|(const Attributes& left, const Attributes& right)
{
    return static_cast<Attributes>(static_cast<int>(left)
                                   | static_cast<int>(right));
}

inline Attributes& operator|=(Attributes& left, const Attributes& right)
{
    return left = left | right;
}

inline Attributes operator&(const Attributes& left, const Attributes& right)
{
    return static_cast<Attributes>(static_cast<int>(left)
                                   & static_cast<int>(right));
}

inline Attributes& operator&=(Attributes& left, const Attributes& right)
{
    return left = left & right;
}

inline Attributes operator^(const Attributes& left, const Attributes& right)
{
    return static_cast<Attributes>(static_cast<int>(left)
                                   ^ static_cast<int>(right));
}

inline Attributes& operator^=(Attributes& left, const Attributes& right)
{
    return left = left ^ right;
}

enum class CachingMode
{
    eUncacheable,
    eUncacheableStrong,
    eWriteThrough,
    eWriteProtected,
    eWriteCombining,
    eWriteBack,
};

template <typename Address>
inline static constexpr bool IsHigherHalfAddress(Address addr)
{
    return reinterpret_cast<VirtAddr>(addr) >= HIGHER_HALF_OFFSET;
}

template <typename T>
inline static constexpr T ToHigherHalfAddress(PhysAddr addr)
{
    T ret = IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(addr + HIGHER_HALF_OFFSET);
    return ret;
}
template <typename T>
inline static constexpr T FromHigherHalfAddress(VirtAddr addr)
{
    T ret = !IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(addr - HIGHER_HALF_OFFSET);
    return ret;
}

namespace Arch
{
    extern uintptr_t pteAddressMask;
    extern uintptr_t newTableFlags;

    extern void*     AllocatePageTable();
} // namespace Arch

struct PageTable;
class PageTableEntry final
{
  public:
    PageTableEntry() = default;

    inline PhysAddr  GetAddress() const { return value & Arch::pteAddressMask; }
    inline bool      GetFlag(uintptr_t flag) const { return value & flag; }
    inline uintptr_t GetFlags() const { return value & ~Arch::pteAddressMask; }

    inline void      SetAddress(uintptr_t address)
    {
        value &= ~Arch::pteAddressMask;
        value |= address;
    }
    inline void SetFlags(uintptr_t flags, bool enabled)
    {
        if (enabled) value |= flags;
        else value &= ~flags;
    }

    inline void Clear() { value = 0; }

    bool        IsValid();
    bool        IsLarge();

  private:
    PhysAddr value;
};

enum class vsptypes
{
    modules,
    uacpi,
    ecam,
    bars,
    other
};

namespace Arch::VMM
{
    usize GetPageSize(Attributes flags);
};

class PageMap;
namespace VirtualMemoryManager
{
    void      Initialize();

    uintptr_t AllocateSpace(vsptypes type, usize increment = 0,
                            usize alignment = 0, bool lower_half = false);

    PageMap*  GetKernelPageMap();
    void      SaveCurrentPageMap(PageMap& out);

    void      LoadPageMap(PageMap& pageMap, bool);
} // namespace VirtualMemoryManager
namespace VMM = VirtualMemoryManager;

class PageMap
{
  public:
    PageMap();
    PageMap(uintptr_t topLevel)
        : topLevel(reinterpret_cast<PageTable*>(topLevel))
    {
    }

    void operator=(PhysAddr topLevel)
    {
        this->topLevel = reinterpret_cast<PageTable*>(topLevel);
    }

    bool MapPage(VirtAddr virtAddr, PhysAddr physAddr, u64 flags) const;

    [[nodiscard]]
    inline bool Exists() const
    {
        return topLevel;
    }
    [[nodiscard]]
    inline PageTable* GetTopLevel() const
    {
        return topLevel;
    }

    inline usize GetPageSize(Attributes flags
                             = static_cast<Attributes>(0)) const
    {
        usize pSize = pageSize;
        if (std::to_underlying(flags) & std::to_underlying(Attributes::eLPage))
            pSize = lPageSize;
        else if (std::to_underlying(flags)
                 & std::to_underlying(Attributes::eLLPage))
            pSize = llPageSize;

        return pSize;
    }
    inline Attributes GetPageSizeFlags(usize pageSize) const
    {
        if (pageSize == lPageSize) return Attributes::eLPage;

        if (pageSize == llPageSize) return Attributes::eLLPage;

        return static_cast<Attributes>(0);
    }

    std::pair<usize, Attributes> RequiredSize(usize size) const
    {
        if (size >= llPageSize) return {llPageSize, Attributes::eLLPage};
        else if (size >= lPageSize) return {lPageSize, Attributes::eLPage};

        return {pageSize, static_cast<Attributes>(0)};
    }

    void* GetNextLevel(PageTableEntry& entry, bool allocate, VirtAddr virt = -1,
                       usize opsize = -1, usize pageSize = -1);

    PageTableEntry* Virt2Pte(PageTable* topLevel, VirtAddr virt, bool allocate,
                             u64 pageSize);
    PhysAddr Virt2Phys(VirtAddr virt, Attributes flags = Attributes::eRead);

    bool     MapNoLock(VirtAddr vaddr, PhysAddr paddr,
                       Attributes  flags = Attributes::eRW,
                       CachingMode cache = CachingMode::eWriteBack);

    bool     UnmapNoLock(VirtAddr   vaddr,
                         Attributes flags = static_cast<Attributes>(0));

    bool Map(VirtAddr vaddr, PhysAddr paddr, Attributes flags = Attributes::eRW,
             CachingMode cache = CachingMode::eWriteBack)
    {
        std::unique_lock guard(this->lock);
        return this->MapNoLock(vaddr, paddr, flags, cache);
    }

    bool Unmap(VirtAddr vaddr, Attributes flags = static_cast<Attributes>(0))
    {
        std::unique_lock guard(this->lock);
        return this->UnmapNoLock(vaddr, flags);
    }

    bool SetFlags(VirtAddr vaddr, Attributes flags = Attributes::eRW,
                  CachingMode cache = CachingMode::eWriteBack);

    bool Remap(VirtAddr vaddr_old, VirtAddr vaddr_new,
               Attributes  flags = Attributes::eRW,
               CachingMode cache = CachingMode::eWriteBack)
    {
        PhysAddr paddr = this->Virt2Phys(vaddr_old, flags);
        this->Unmap(vaddr_old, flags);
        return this->Map(vaddr_new, paddr, flags, cache);
    }

    bool MapRange(VirtAddr vaddr, PhysAddr paddr, usize size,
                  Attributes  flags = Attributes::eRW,
                  CachingMode cache = CachingMode::eWriteBack)
    {
        usize psize = this->GetPageSize(flags);
        for (usize i = 0; i < size; i += psize)
        {
            if (!this->Map(vaddr + i, paddr + i, flags, cache))
            {
                this->UnmapRange(vaddr, i - psize);
                return false;
            }
        }
        return true;
    }

    bool UnmapRange(VirtAddr vaddr, usize size,
                    Attributes flags = static_cast<Attributes>(0))
    {
        usize psize = this->GetPageSize(flags);
        for (usize i = 0; i < size; i += psize)
            if (!this->Unmap(vaddr + i, flags)) return false;

        return true;
    }

    bool RemapRange(VirtAddr vaddr_old, VirtAddr vaddr_new, usize size,
                    Attributes  flags = Attributes::eRW,
                    CachingMode cache = CachingMode::eWriteBack)
    {
        usize psize = this->GetPageSize(flags);
        for (usize i = 0; i < size; i += psize)
            if (!this->Remap(vaddr_old + i, vaddr_new + i, flags, cache))
                return false;

        return true;
    }

    bool SetFlagsRange(VirtAddr vaddr, usize size,
                       Attributes  flags = Attributes::eRW,
                       CachingMode cache = CachingMode::eWriteBack)
    {
        usize psize = this->GetPageSize(flags);
        for (usize i = 0; i < size; i += psize)
            if (!this->SetFlags(vaddr + i, flags, cache)) return false;

        return true;
    }

    inline void Load() { VirtualMemoryManager::LoadPageMap(*this, true); }

  private:
    PageTable* topLevel = 0;
    std::mutex lock;
    usize      pageSize   = 0;
    usize      lPageSize  = 0;
    usize      llPageSize = 0;
};

bool                               IsCanonical(uintptr_t addr);
uintptr_t                          Flags2Arch(Attributes flags);
std::pair<Attributes, CachingMode> Arch2Flags(uintptr_t flags, bool lpages);
