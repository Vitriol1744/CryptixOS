/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/x86_64/IO.hpp>
#include <Firmware/ACPI/ACPI.hpp>

#include <Memory/MMIO.hpp>
#include <Time/Time.hpp>
#include <Utility/Math.hpp>
#include <Utility/Spinlock.hpp>

#include <uacpi/kernel_api.h>

namespace uACPI
{
    extern "C"
    {
        uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr* outRsdp)
        {
            uacpi_phys_addr rsdp = BootInfo::GetRSDPAddress();

            if (rsdp)
            {
                *outRsdp = rsdp;
                return UACPI_STATUS_OK;
            }

            return UACPI_STATUS_NOT_FOUND;
        }

        uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address,
                                                  uacpi_u8        byteWidth,
                                                  uacpi_u64*      outValue)
        {
            uacpi_status status = UACPI_STATUS_OK;
            void*        virt   = uacpi_kernel_map(address, byteWidth);

            switch (byteWidth)
            {
                case 1: *outValue = MMIO::Read<u8>(virt); break;
                case 2: *outValue = MMIO::Read<u16>(virt); break;
                case 4: *outValue = MMIO::Read<u32>(virt); break;
                case 8: *outValue = MMIO::Read<u64>(virt); break;

                default: status = UACPI_STATUS_INVALID_ARGUMENT; break;
            }

            uacpi_kernel_unmap(virt, byteWidth);
            return status;
        }
        uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address,
                                                   uacpi_u8        byteWidth,
                                                   uacpi_u64       inValue)
        {
            uacpi_status status = UACPI_STATUS_OK;
            void*        virt   = uacpi_kernel_map(address, byteWidth);
            switch (byteWidth)
            {
                case 1: MMIO::Write<u8>(virt, inValue); break;
                case 2: MMIO::Write<u16>(virt, inValue); break;
                case 4: MMIO::Write<u32>(virt, inValue); break;
                case 8: MMIO::Write<u64>(virt, inValue); break;

                default: status = UACPI_STATUS_INVALID_ARGUMENT; break;
            }

            uacpi_kernel_unmap(virt, byteWidth);
            return status;
        }

        uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr address,
                                              uacpi_u8      byteWidth,
                                              uacpi_u64*    outValue)
        {
            u16 port = address;
            switch (byteWidth)
            {
                case 1: *outValue = IO::In<u8>(port); break;
                case 2: *outValue = IO::In<u16>(port); break;
                case 4: *outValue = IO::In<u32>(port); break;
                default: return UACPI_STATUS_INVALID_ARGUMENT;
            }

            return UACPI_STATUS_OK;
        }
        uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr address,
                                               uacpi_u8      byteWidth,
                                               uacpi_u64     inValue)
        {
            u16 port = address;
            switch (byteWidth)
            {
                case 1: IO::Out<u8>(port, inValue); break;
                case 2: IO::Out<u16>(port, inValue); break;
                case 4: IO::Out<u32>(port, inValue); break;
                default: return UACPI_STATUS_INVALID_ARGUMENT;
            }

            return UACPI_STATUS_OK;
        }

        uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address,
                                                  uacpi_handle*     outHandle)
        {
            CtosUnused(address);
            CtosUnused(outHandle);

            return UACPI_STATUS_UNIMPLEMENTED;
        }
        void         uacpi_kernel_pci_device_close(uacpi_handle) {}

        uacpi_status uacpi_kernel_pci_read(uacpi_handle device,
                                           uacpi_size   offset,
                                           uacpi_u8 byteWidth, uacpi_u64* value)
        {

            CtosUnused(device);
            CtosUnused(offset);
            CtosUnused(byteWidth);
            CtosUnused(value);

            ToDo();
            return UACPI_STATUS_UNIMPLEMENTED;
        }
        uacpi_status uacpi_kernel_pci_write(uacpi_handle device,
                                            uacpi_size   offset,
                                            uacpi_u8 byteWidth, uacpi_u64 value)
        {
            CtosUnused(device);
            CtosUnused(offset);
            CtosUnused(byteWidth);
            CtosUnused(value);

            ToDo();
            return UACPI_STATUS_UNIMPLEMENTED;
        }

        uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len,
                                         uacpi_handle* outHandle)
        {
            *outHandle = reinterpret_cast<uacpi_handle>(base);

            return UACPI_STATUS_OK;
        }
        void         uacpi_kernel_io_unmap(uacpi_handle) {}

        uacpi_status uacpi_kernel_io_read(uacpi_handle handle,
                                          uacpi_size offset, uacpi_u8 byteWidth,
                                          uacpi_u64* value)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_read(addr + offset, byteWidth, value);
        }
        uacpi_status uacpi_kernel_io_write(uacpi_handle handle,
                                           uacpi_size   offset,
                                           uacpi_u8 byteWidth, uacpi_u64 value)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_write(addr + offset, byteWidth, value);
        }

        void* uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
        {
            PageMap*  pageMap = VMM::GetKernelPageMap();

            uintptr_t phys    = Math::AlignDown(addr, PMM::PAGE_SIZE);
            uintptr_t virt    = VMM::AllocateSpace(len, PMM::PAGE_SIZE, true);
            len = Math::AlignUp(addr - phys + len, PMM::PAGE_SIZE);
            pageMap->MapRange(virt, phys, len);
            return reinterpret_cast<u8*>(virt) + (addr - phys);
        }
        void uacpi_kernel_unmap(void* addr, uacpi_size len)
        {
            PageMap*  pageMap = VMM::GetKernelPageMap();
            uintptr_t virt = Math::AlignDown(reinterpret_cast<uintptr_t>(addr),
                                             PMM::PAGE_SIZE);
            len = Math::AlignUp(reinterpret_cast<uintptr_t>(addr) - virt + len,
                                PMM::PAGE_SIZE);

            pageMap->UnmapRange(virt, len);
        }

        void* uacpi_kernel_alloc(uacpi_size size) { return new u8[size]; }
        void* uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
        {
            return new u8[count * size];
        }

        void uacpi_kernel_free(void* memory)
        {
            delete[] reinterpret_cast<u8*>(memory);
        }

        void uacpi_kernel_log(uacpi_log_level level, const uacpi_char* message)
        {
            switch (level)
            {
                case UACPI_LOG_DEBUG: LogDebug("{}", message); break;
                case UACPI_LOG_TRACE: LogTrace("{}", message); break;
                case UACPI_LOG_INFO: LogInfo("{}", message); break;
                case UACPI_LOG_WARN: LogWarn("{}", message); break;
                case UACPI_LOG_ERROR: LogError("{}", message); break;
            }
        }

        uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void)
        {
            return Time::GetEpoch() - BootInfo::GetBootTime();
        }
        void uacpi_kernel_stall(uacpi_u8 usec)
        {
            CtosUnused(usec);
            ToDo();
        }
        void uacpi_kernel_sleep(uacpi_u64 msec)
        {
            CtosUnused(msec);
            ToDo();
        }

        uacpi_handle uacpi_kernel_create_mutex(void)
        {
            ToDo();
            return {};
        }
        void         uacpi_kernel_free_mutex(uacpi_handle) { ToDo(); }

        uacpi_handle uacpi_kernel_create_event(void)
        {
            ToDo();
            return {};
        }
        void            uacpi_kernel_free_event(uacpi_handle) { ToDo(); }

        uacpi_thread_id uacpi_kernel_get_thread_id(void)
        {
            ToDo();

            return 0;
        }

        uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16)
        {
            ToDo();
            return UACPI_STATUS_UNIMPLEMENTED;
        }
        void       uacpi_kernel_release_mutex(uacpi_handle) { ToDo(); }

        uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16)
        {
            ToDo();
            return false;
        }
        void uacpi_kernel_signal_event(uacpi_handle) { ToDo(); }
        void uacpi_kernel_reset_event(uacpi_handle) { ToDo(); }
        uacpi_status
        uacpi_kernel_handle_firmware_request(uacpi_firmware_request*)
        {
            ToDo();
            return UACPI_STATUS_UNIMPLEMENTED;
        }

        uacpi_status uacpi_kernel_install_interrupt_handler(
            uacpi_u32 irq, uacpi_interrupt_handler, uacpi_handle ctx,
            uacpi_handle* outIrqHandle)
        {
            CtosUnused(irq);
            CtosUnused(ctx);
            CtosUnused(outIrqHandle);

            ToDo();
            return UACPI_STATUS_UNIMPLEMENTED;
        }

        uacpi_status
        uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler,
                                                 uacpi_handle irqHandle)
        {
            CtosUnused(irqHandle);
            ToDo();
            return UACPI_STATUS_UNIMPLEMENTED;
        }

        uacpi_handle uacpi_kernel_create_spinlock(void)
        {
            Spinlock* spinlock = new Spinlock;

            return reinterpret_cast<uacpi_handle>(spinlock);
        }
        void uacpi_kernel_free_spinlock(uacpi_handle spinlock)
        {
            delete reinterpret_cast<Spinlock*>(spinlock);
        }

        uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle)
        {
            uacpi_cpu_flags interruptState = CPU::GetInterruptFlag();

            reinterpret_cast<Spinlock*>(handle)->Acquire();
            return interruptState;
        }
        void uacpi_kernel_unlock_spinlock(uacpi_handle    spinlock,
                                          uacpi_cpu_flags interruptState)
        {
            reinterpret_cast<Spinlock*>(spinlock)->Release();

            CPU::SetInterruptFlag(interruptState);
        }

        uacpi_status uacpi_kernel_schedule_work(uacpi_work_type,
                                                uacpi_work_handler,
                                                uacpi_handle ctx)
        {
            CtosUnused(ctx);
            ToDo();

            return UACPI_STATUS_UNIMPLEMENTED;
        }
        uacpi_status uacpi_kernel_wait_for_work_completion(void)
        {
            ToDo();

            return UACPI_STATUS_UNIMPLEMENTED;
        }
    };
}; // namespace uACPI
