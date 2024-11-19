/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "ACPI/ACPI.hpp"
#include "Arch/Arch.hpp"
#include "Arch/x86_64/CPU.hpp"

#include "Drivers/Serial.hpp"
#include "Drivers/Terminal.hpp"
#include "Memory/PhysicalMemoryManager.hpp"
#include "Memory/VirtualMemoryManager.hpp"
#include "Utility/BootInfo.hpp"
#include "Utility/ELFLoader.hpp"
#include "Utility/Logger.hpp"
#include "Utility/Stacktrace.hpp"
#include "Utility/Types.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Scheduler.hpp"
#include "Scheduler/Thread.hpp"

#include <limine.h>
#include <stdint.h>

extern "C" void __assert_fail(const char* expr, const char* file,
                              unsigned int line, const char* function)
{
    __builtin_unreachable();
}

static Process* kernelProcess = nullptr;

namespace
{

    [[maybe_unused]]
    void hcf()
    {
        for (;;)
        {
#if defined(__x86_64__)
            asm("hlt");
#elif defined(__aarch64__) || defined(__riscv)
            asm("wfi");
#elif defined(__loongarch64)
            asm("idle 0");
#endif
        }
    }

} // namespace
using ConstructorFunction = void (*)();

extern ConstructorFunction       __init_array_start[];
extern ConstructorFunction       __init_array_end[];

void                             kernelThread()
{
    LogInfo("KernelThread");

    Arch::Halt();
}

extern "C" void kernelStart()
{
    Serial::Initialize();
    Logger::SetLogOutput(LOG_OUTPUT_SERIAL);

    PhysicalMemoryManager::Initialize();
    Logger::InitializeTerminal();

    for (ConstructorFunction* entry = __init_array_start;
         entry < __init_array_end; entry++)
    {
        ConstructorFunction constructor = *entry;
        constructor();
    }

    LogInfo("Boot: Kernel loaded with {}-{} at {}s",
            BootInfo::GetBootloaderName(), BootInfo::GetBootloaderVersion(),
            BootInfo::GetBootTime());

    LogInfo("Boot: Kernel Physical Address: 0x{:x}",
            BootInfo::GetKernelPhysicalAddress());
    LogInfo("Boot: Kernel Virtual Address: 0x{:x}",
            BootInfo::GetKernelVirtualAddress());

    VirtualMemoryManager::Initialize();
    ACPI::Initialize();
    Arch::Initialize();
    if (!Stacktrace::Initialize())
        LogWarn("Stacktrace: Failed to load kernel symbols");

    kernelProcess          = new ::Process("Kernel Process");
    kernelProcess->pageMap = VMM::GetKernelPageMap();
    Scheduler::EnqueueThread(
        new Thread(kernelProcess, reinterpret_cast<uintptr_t>(&kernelThread), 0,
                   CPU::GetCurrent()->id));

    LogInfo("Scheduler: Initialized");
    __asm__ volatile("sti");

    Scheduler::PrepareAP(true);
}
