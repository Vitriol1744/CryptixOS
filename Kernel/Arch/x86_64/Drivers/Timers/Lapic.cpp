/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Lapic.hpp"

#include "ACPI/ACPI.hpp"
#include "Arch/x86_64/CPU.hpp"
#include "Memory/VirtualMemoryManager.hpp"

constexpr u32             LAPIC_EOI_ACK                      = 0x00;
CTOS_UNUSED constexpr u32 APIC_BASE_MSR                      = 0x1b;

CTOS_UNUSED constexpr u32 LAPIC_ID_REGISTER                  = 0x20;
CTOS_UNUSED constexpr u32 LAPIC_VERSION_REGISTER             = 0x30;
CTOS_UNUSED constexpr u32 LAPIC_TASK_PRIORITY_REGISTER       = 0x80;
constexpr u32             LAPIC_EOI_REGISTER                 = 0xb0;
CTOS_UNUSED constexpr u32 LAPIC_LDR_REGISTER                 = 0xd0;
CTOS_UNUSED constexpr u32 LAPIC_DFR_REGISTER                 = 0xe0;
CTOS_UNUSED constexpr u32 LAPIC_SPURIOUS_REGISTER            = 0xf0;
CTOS_UNUSED constexpr u32 LAPIC_ESR_REGISTER                 = 0x280;
constexpr u32             LAPIC_ICR_LOW_REGISTER             = 0x300;
constexpr u32             LAPIC_ICR_HIGH_REGISTER            = 0x310;
constexpr u32             LAPIC_TIMER_REGISTER               = 0x320;
constexpr u32             LAPIC_TIMER_INITIAL_COUNT_REGISTER = 0x380;
constexpr u32             LAPIC_TIMER_CURRENT_COUNT_REGISTER = 0x390;
constexpr u32             LAPIC_TIMER_DIVIDER_REGISTER       = 0x3e0;

CTOS_UNUSED constexpr u32 LAPIC_TIMER_MASKED                 = 0x10000;
CTOS_UNUSED constexpr u32 LAPIC_TIMER_PERIODIC               = 0x20000;

bool                      CheckX2Apic()
{

    // TODO(v1tr10l7): Check for X2 apic
    u64 rax, rbx, rcx, rdx;
    if (!CPU::ID(1, 0, rax, rbx, rcx, rdx)) return false;
    if (!(rcx & BIT(21))) return false;
    auto dmar = ACPI::GetTable("DMAR");
    if (!dmar) return true;

    return false;
}

void Lapic::Initialize()
{
    x2apic    = CheckX2Apic();

    auto base = CPU::ReadMSR(APIC_BASE_MSR) | BIT(11);
    if (x2apic) base |= BIT(10);

    CPU::WriteMSR(APIC_BASE_MSR, base);

    if (!x2apic)
    {
        auto physAddr = base & ~(0xfff);

        baseAddress
            = VMM::AllocateSpace(vsptypes::other, 0x1000, sizeof(u32), true);
        VMM::GetKernelPageMap()->Map(baseAddress, physAddr, Attributes::eRW,
                                     CachingMode::eUncacheableStrong);
    }

    baseAddress = ToHigherHalfAddress<uintptr_t>(base & ~(0xfff));
    id          = x2apic ? Read(LAPIC_ID_REGISTER)
                         : (Read(LAPIC_ID_REGISTER) >> 24) & 0xff;

    Write(LAPIC_TASK_PRIORITY_REGISTER, 0x00);
    Write(LAPIC_SPURIOUS_REGISTER, 0xff | BIT(8));
};

void Lapic::SendIpi(u32 flags, u32 id)
{
    if (!x2apic)
    {
        Write(LAPIC_ICR_HIGH_REGISTER, id << 24);
        Write(LAPIC_ICR_LOW_REGISTER, flags);
        return;
    }

    Write(LAPIC_ICR_LOW_REGISTER,
          (static_cast<uint64_t>(id) << 32) | BIT(14) | flags);
}
void Lapic::SendEOI() { Write(LAPIC_EOI_REGISTER, LAPIC_EOI_ACK); }

void Lapic::Start(u8 vector, u64 ms, Mode mode)
{
    if (ticksPerMs == 0) CalibrateTimer();

    u64 ticks = ticksPerMs * ms;

    Write(LAPIC_TIMER_DIVIDER_REGISTER, 0x03);
    u64 value = Read(LAPIC_TIMER_REGISTER) & ~(3 << 17);

    value |= std::to_underlying(mode) << 17;
    value &= 0xffffff00;
    value |= vector;
    Write(LAPIC_TIMER_REGISTER, value);
    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, ticks ? ticks : 1);
    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) & ~(1 << 16));
}

u32 Lapic::Read(u32 reg)
{
    if (x2apic) return CPU::ReadMSR((reg >> 4) + 0x800);

    volatile auto ptr = reinterpret_cast<volatile u32*>(baseAddress);
    return *ptr;
}

void Lapic::Write(u32 reg, u64 value)
{
    if (x2apic) return CPU::WriteMSR((reg >> 4) + 0x800, value);

    volatile auto ptr = reinterpret_cast<volatile u32*>(baseAddress + reg);
    *ptr              = value;
}

void Lapic::CalibrateTimer()
{
    Write(LAPIC_TIMER_DIVIDER_REGISTER, 0x03);
    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0xffffffff);

    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) & ~BIT(16));

    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) | BIT(16));
    ticksPerMs = (0xffffffff - Read(LAPIC_TIMER_CURRENT_COUNT_REGISTER)) / 10;
}
