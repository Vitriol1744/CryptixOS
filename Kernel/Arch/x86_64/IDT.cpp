/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "IDT.hpp"

#include "Common.hpp"

#include "ACPI/MADT.hpp"
#include "Arch/Interrupts/InterruptHandler.hpp"
#include "Arch/x86_64/CPU.hpp"
#include "Arch/x86_64/GDT.hpp"

extern const char*         exceptionNames[];

inline constexpr const u32 MAX_IDT_ENTRIES   = 256;
inline constexpr const u32 IDT_ENTRY_PRESENT = BIT(7);

#pragma pack(push, 1)
struct IDTEntry
{
    u16 isrLow;
    u16 kernelCS;
    u8  ist;
    union
    {
        u8 attributes;
        struct
        {
            u8 gateType : 4;
            u8 unused   : 1;
            u8 dpl      : 2;
            u8 present  : 1;
        };
    };
    u16 isrMiddle;
    u32 isrHigh;
    u32 reserved;
};
#pragma pack(pop)

inline constexpr const uint32_t GATE_TYPE_INTERRUPT            = 0xe;
inline constexpr const uint32_t GATE_TYPE_TRAP                 = 0xf;

[[maybe_unused]] alignas(0x10) static IDTEntry idtEntries[256] = {};
static InterruptHandler interruptHandlers[256];

extern "C" void*        interrupt_handlers[];
static void             PanicHandler(CPUContext*)
{
    LogError("CPU[{}] halted.", CPU::GetCurrent()->id);
    Arch::Halt();
}

static void idtWriteEntry(uint16_t vector, uintptr_t handler,
                          uint8_t attributes)
{
    Assert(vector <= 256);

    IDTEntry* entry   = idtEntries + vector;

    entry->isrLow     = handler & 0xffff;
    entry->kernelCS   = GDT::KERNEL_CODE_SELECTOR;
    entry->attributes = attributes;
    entry->reserved   = 0;
    entry->ist        = 0;
    entry->isrMiddle  = (handler & 0xffff0000) >> 16;
    entry->isrHigh    = (handler & 0xffffffff00000000) >> 32;
}

[[noreturn]] [[maybe_unused]]
static void raiseException(CPUContext* ctx)
{
    LogError("{}", exceptionNames[ctx->interruptVector]);
    Panic("Captured exception on cpu {}: '{}'\n\rError Code: {}\n\rrip: {}", 0,
          exceptionNames[ctx->interruptVector], ctx->errorCode, ctx->rip);
}

[[noreturn]]
static void unhandledInterrupt(CPUContext* context)
{
    LogError("\nAn unhandled interrupt 0x{:x} occurred",
             context->interruptVector);

    for (;;) { __asm__ volatile("cli; hlt"); }
}
extern "C" void raiseInterrupt(CPUContext* ctx)
{
    if (ctx->interruptVector < 0x20) raiseException(ctx);
    else if (interruptHandlers[ctx->interruptVector].IsUsed())
    {
        interruptHandlers[ctx->interruptVector](ctx);
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
            idtWriteEntry(i, reinterpret_cast<uintptr_t>(interrupt_handlers[i]),
                          0x80 | GATE_TYPE_INTERRUPT);
            interruptHandlers[i].SetInterruptVector(i);
        }

        interruptHandlers[0xff].SetHandler(PanicHandler);
        interruptHandlers[0xff].Reserve();
        LogInfo("IDT Initialized!");
    }
    void Load()
    {
#pragma pack(push, 1)
        struct
        {
            u16       limit;
            uintptr_t base;
        } idtr;
#pragma pack(pop)
        idtr.limit = sizeof(idtEntries) - 1;
        idtr.base  = reinterpret_cast<uintptr_t>(idtEntries);
        __asm__ volatile("lidt %0" : : "m"(idtr));
    }

    void SetIST(u8 vector, u32 value) { idtEntries[vector].ist = value; }

    using InterruptHandlerFunction = void (*)(CPUContext*);
    InterruptHandler*                     AllocateHandler(u8 hint)
    {
        LogTrace("IDT: Allocating handler...");
        if (hint < 0x20) hint += 0x20;

        if (MADT::LegacyPIC())
        {
            if ((hint >= 0x20 && hint <= (0x20 + 15))
                && !interruptHandlers[hint].IsUsed())
                return &interruptHandlers[hint];
        }

        for (size_t i = hint; i < 256; i++)
        {
            if (!interruptHandlers[i].IsUsed()
                && !interruptHandlers[i].IsReserved())
            {
                interruptHandlers[i].Reserve();
                interruptHandlers[i].SetInterruptVector(i);
                return &interruptHandlers[i];
            }
        }

        Panic("IDT: Out of interrupt handlers");
    }
    InterruptHandler& GetHandler(u8 vector)
    {
        return interruptHandlers[vector];
    }

    void RegisterInterruptHandler(u32 vector, InterruptHandlerFunction handler,
                                  u8 dpl)
    {
        Assert(vector <= 256);
        //        interruptHandlers[vector] = handler;
        idtEntries[vector].dpl = dpl;
    }
    void RegisterInterruptHandler(InterruptHandler* handler, u8 dpl)
    {
        Assert(handler->GetInterruptVector() >= 0x20);
        interruptHandlers[handler->GetInterruptVector()] = *handler;
        idtEntries[handler->GetInterruptVector()].dpl    = dpl;
    }
}; // namespace IDT

#pragma region exception_names
const char*    exceptionNames[] = {
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
