/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/CPUContext.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Arch/InterruptHandler.hpp>

#include <Firmware/ACPI/MADT.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

extern const char* s_ExceptionNames[];

constexpr u32      MAX_IDT_ENTRIES     = 256;
constexpr u32      IDT_ENTRY_PRESENT   = BIT(7);

constexpr u32      GATE_TYPE_INTERRUPT = 0xe;
constexpr u32      GATE_TYPE_TRAP      = 0xf;

namespace Exception
{
    constexpr u8 BREAKPOINT = 0x03;
    constexpr u8 PAGE_FAULT = 0x0e;
}; // namespace Exception

struct [[gnu::packed]] IDTEntry
{
    u16 IsrLow;
    u16 KernelCS;
    u8  IST;
    union
    {
        u8 Attributes;
        struct [[gnu::packed]]
        {
            u8 GateType : 4;
            u8 Unused   : 1;
            u8 DPL      : 2;
            u8 Present  : 1;
        };
    };
    u16 IsrMiddle;
    u32 IsrHigh;
    u32 Reserved;
};

[[maybe_unused]] alignas(0x10) static IDTEntry s_IdtEntries[256] = {};
extern "C" void*        interrupt_handlers[];
static InterruptHandler s_InterruptHandlers[256];
static void             (*exceptionHandlers[32])(CPUContext*);

static void idtWriteEntry(u16 vector, uintptr_t handler, u8 attributes)
{
    Assert(vector < MAX_IDT_ENTRIES);
    IDTEntry* entry   = s_IdtEntries + vector;

    entry->IsrLow     = handler & 0xffff;
    entry->KernelCS   = GDT::KERNEL_CODE_SELECTOR;
    entry->Attributes = attributes;
    entry->Reserved   = 0;
    entry->IST        = 0;
    entry->IsrMiddle  = (handler & 0xffff0000) >> 16;
    entry->IsrHigh    = (handler & 0xffffffff00000000) >> 32;
}

[[noreturn]] static void raiseException(CPUContext* ctx)
{
    u64 cpuID = CPU::GetCurrentID();

    EarlyPanic(
        "Captured exception[%#x] on cpu %zu: '%s'\n\rError Code: "
        "%#b\n\rrip: "
        "%#p",
        ctx->interruptVector, cpuID, s_ExceptionNames[ctx->interruptVector],
        ctx->errorCode, ctx->rip);

    Arch::Halt();
}

// TODO(v1tr10l7): properly handle breakpoints and page faults
static void breakpoint(CPUContext* ctx) { EarlyPanic("Breakpoint"); }
static void pageFault(CPUContext* ctx)
{
    CPU::DumpRegisters(ctx);
    EarlyPanic(
        "Captured exception[%#zx] on cpu %zu: '%s'\n\rPresent: %d\n Write: "
        "%d\n, USER: %d\n, ReservedWrite: "
        "%d\n, InstructionFetch: %d\n, ProtectionKey: %d\n, ShadowStack: %d\n, "
        "SoftwareGuardExtensions: %d\n"
        "\nerrorCode: %#zb\n\rrip: "
        "%#p\nCR2: %#zx",
        ctx->interruptVector, 0, s_ExceptionNames[ctx->interruptVector],
        (ctx->errorCode & Bit(0)) != 0, (ctx->errorCode & Bit(1)) != 0,
        (ctx->errorCode & Bit(2)) != 0, (ctx->errorCode & Bit(3)) != 0,
        (ctx->errorCode & Bit(4)) != 0, (ctx->errorCode & Bit(5)) != 0,
        (ctx->errorCode & Bit(6)) != 0, (ctx->errorCode & Bit(7)) != 0,
        ctx->errorCode, ctx->rip, CPU::ReadCR2());

    for (;;) Arch::Halt();
    raiseException(ctx);
}

[[noreturn]] static void unhandledInterrupt(CPUContext* context)
{
    EarlyLogError("\nAn unhandled interrupt %#x occurred",
                  context->interruptVector);

    for (;;) __asm__ volatile("cli; hlt");
}
extern "C" void raiseInterrupt(CPUContext* ctx)
{
    auto& handler = s_InterruptHandlers[ctx->interruptVector];

    if (ctx->interruptVector < 0x20)
        exceptionHandlers[ctx->interruptVector](ctx);
    else if (handler.IsUsed())
    {
        if (handler.eoiFirst) InterruptManager::SendEOI(ctx->interruptVector);
        handler(ctx);
        if (!handler.eoiFirst) InterruptManager::SendEOI(ctx->interruptVector);

        return;
    }

    unhandledInterrupt(ctx);
}

namespace IDT
{
    void Initialize()
    {
        for (u32 i = 0; i < 256; i++)
        {

            idtWriteEntry(
                i, reinterpret_cast<uintptr_t>(interrupt_handlers[i]),
                IDT_ENTRY_PRESENT
                    | (i < 0x20 ? GATE_TYPE_TRAP : GATE_TYPE_INTERRUPT));

            s_InterruptHandlers[i].SetInterruptVector(i);
            if (i < 32) exceptionHandlers[i] = raiseException;
        }

        exceptionHandlers[Exception::BREAKPOINT] = breakpoint;
        exceptionHandlers[Exception::PAGE_FAULT] = pageFault;

        SetDPL(Exception::BREAKPOINT, 3);
        LogInfo("IDT: Initialized!");
    }
    void Load()
    {
        struct [[gnu::packed]]
        {
            u16       Limit;
            uintptr_t Base;
        } idtr;
        idtr.Limit = sizeof(s_IdtEntries) - 1;
        idtr.Base  = reinterpret_cast<uintptr_t>(s_IdtEntries);
        __asm__ volatile("lidt %0" : : "m"(idtr));
    }

    InterruptHandler* AllocateHandler(u8 hint)
    {
        LogTrace("IDT: Allocating handler...");
        if (hint < 0x20) hint += 0x20;

        if (MADT::LegacyPIC())
        {
            if ((hint >= 0x20 && hint <= (0x20 + 15))
                && !s_InterruptHandlers[hint].IsUsed())
                return &s_InterruptHandlers[hint];
        }

        for (usize i = hint; i < 256; i++)
        {
            if (!s_InterruptHandlers[i].IsUsed()
                && !s_InterruptHandlers[i].IsReserved())
            {
                s_InterruptHandlers[i].Reserve();
                s_InterruptHandlers[i].SetInterruptVector(i);
                return &s_InterruptHandlers[i];
            }
        }

        Panic("IDT: Out of interrupt handlers");
    }
    InterruptHandler* GetHandler(u8 vector)
    {
        return &s_InterruptHandlers[vector];
    }

    void SetIST(u8 vector, u32 value) { s_IdtEntries[vector].IST = value; }
    void SetDPL(u8 vector, u8 dpl) { s_IdtEntries[vector].DPL = dpl; }
} // namespace IDT

#pragma region exception_names
const char*    s_ExceptionNames[] = {
    "Divide-by-zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device not available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved",
    "Triple Fault",
    "FPU Error Interrupt",
};
#pragma endregion
