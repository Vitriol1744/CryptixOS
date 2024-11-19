/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "IoApic.hpp"

#include "ACPI/MADT.hpp"
#include "Arch/x86_64/Drivers/PIC.hpp"
#include "Arch/x86_64/IDT.hpp"

#include "Memory/VirtualMemoryManager.hpp"

static std::vector<IoApic>       ioapics;

[[maybe_unused]] constexpr usize IOAPICID_REGISTER  = 0;
[[maybe_unused]] constexpr usize IOAPICVER_REGISTER = 1;
[[maybe_unused]] constexpr usize IOAPICARB_REGISTER = 2;

IoApic::IoApic(uintptr_t baseAddress, u32 gsiBase)
    : baseAddress(baseAddress)
    , gsiBase(gsiBase)
{
    this->baseAddress
        = VMM::AllocateSpace(vsptypes::other, 0x1000, sizeof(u32), true);
    VMM::GetKernelPageMap()->Map(this->baseAddress, baseAddress,
                                 Attributes::eRW,
                                 CachingMode::eUncacheableStrong);

    redirectionEntryCount = ((Read(IOAPICVER_REGISTER) >> 16) & 0xff) + 1;
    for (u64 i = 1; i < redirectionEntryCount; i++)
    {

        auto entry = GetRedirectionEntry(i) | BIT(16);
        SetRedirectionEntry(i, entry);
    }
}

void IoApic::RedirectIRQ(u32 gsi, IoApicRedirectionEntry& entry)
{
    auto ioapic = GetIoApicForGsi(gsi);
    ioapic->SetRedirectionEntry(gsi - ioapic->gsiBase, entry);
}
void IoApic::Mask(u32 gsi)
{
    auto ioapic     = GetIoApicForGsi(gsi);

    u64  entryValue = ioapic->GetRedirectionEntry(gsi - ioapic->gsiBase);
    IoApicRedirectionEntry entry
        = *reinterpret_cast<IoApicRedirectionEntry*>(&entryValue);

    entry.flags = entry.flags | IoApicRedirectionFlags::eMasked;
    ioapic->SetRedirectionEntry(gsi - ioapic->gsiBase, entry);
}
void IoApic::Unmask(u32 gsi)
{
    auto ioapic     = GetIoApicForGsi(gsi);

    u64  entryValue = ioapic->GetRedirectionEntry(gsi - ioapic->gsiBase);
    IoApicRedirectionEntry entry
        = *reinterpret_cast<IoApicRedirectionEntry*>(&entryValue);

    entry.flags = entry.flags & ~IoApicRedirectionFlags::eMasked;
    ioapic->SetRedirectionEntry(gsi - ioapic->gsiBase, entry);
}

void IoApic::MaskIRQ(u32 irq)
{
    for (const auto& iso : MADT::GetISOEntries())
    {
        if (iso->irqSource == irq)
        {
            Mask(iso->gsi);
            return;
        }
    }

    Mask(irq);
}
void IoApic::UnmaskIRQ(u32 irq)
{
    for (const auto& iso : MADT::GetISOEntries())
    {
        if (iso->irqSource == irq)
        {
            Unmask(iso->gsi);
            return;
        }
    }

    Unmask(irq);
}

void IoApic::Initialize()
{
    LogTrace("IoApic: Initializing...");
    if (MADT::GetIOAPICEntries().empty()) Panic("IOAPIC: MADT not found");
    PIC::MaskAllIRQs();

    for (const auto& entry : MADT::GetIOAPICEntries())
        ioapics.push_back({entry->address, entry->gsib});

    auto redirect = [](usize i)
    {
        for (const auto& iso : MADT::GetISOEntries())
        {
            if (iso->irqSource != i) continue;

            IoApicRedirectionEntry entry;
            entry.vector          = iso->irqSource + 0x20;
            entry.deliveryMode    = DeliveryMode::eFixed;
            entry.destinationMode = DestinationMode::ePhysical;
            entry.flags = static_cast<IoApicRedirectionFlags>(iso->flags)
                        | IoApicRedirectionFlags::eMasked;
            entry.destination = BootInfo::GetSMP_Response()->bsp_lapic_id;

            RedirectIRQ(iso->gsi, entry);
            IDT::GetHandler(i).Reserve();
            return;
        }

        IoApicRedirectionEntry entry;
        entry.vector          = i + 0x20;
        entry.deliveryMode    = DeliveryMode::eFixed;
        entry.destinationMode = DestinationMode::ePhysical;
        entry.flags           = IoApicRedirectionFlags::eMasked;
        entry.destination     = BootInfo::GetSMP_Response()->bsp_lapic_id;

        RedirectIRQ(i, entry);
        IDT::GetHandler(i).Reserve();
    };

    if (!MADT::LegacyPIC()) return;
    for (usize i = 0; i < 16; i++)
    {
        if (i == 2) continue;
        redirect(i);
    }
}

IoApic* IoApic::GetIoApicForGsi(u32 gsi)
{
    for (auto& ioapic : ioapics)
    {
        if (ioapic.gsiBase <= gsi
            && (ioapic.gsiBase + ioapic.redirectionEntryCount) >= gsi)
            return &ioapic;
    }

    return nullptr;
}

u32 IoApic::Read(u32 reg)
{
    u32 volatile* ioapic = (u32 volatile*)baseAddress;
    ioapic[0]            = (reg & 0xff);

    return ioapic[4];
}
void IoApic::Write(u32 reg, u32 value)
{
    u32 volatile* ioapic = (u32 volatile*)baseAddress;
    ioapic[0]            = (reg & 0xff);

    ioapic[4]            = value;
}

u64 IoApic::GetRedirectionEntry(u32 index)
{
    u32 reg = 0x10 + (index * 2);

    return Read(reg) | (static_cast<u64>(Read(reg + 0x01)) << 32);
}
void IoApic::SetRedirectionEntry(u32 index, u64 value)
{
    u32 reg = 0x10 + (index * 2);
    Write(reg, value);
    Write(reg + 0x01, value >> 32);
}

void IoApic::SetRedirectionEntry(u32 index, const IoApicRedirectionEntry& entry)
{
    u64 value = *(reinterpret_cast<const u64*>(&entry));
    SetRedirectionEntry(index, value);
}
