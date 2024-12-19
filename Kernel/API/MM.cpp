/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/MM.hpp>
#include <API/UnixTypes.hpp>

#include <Arch/CPU.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Math.hpp>

namespace Syscall::MM
{
    uintptr_t SysMMap(Syscall::Arguments& args)
    {
        uintptr_t addr    = args.args[0];
        usize     len     = args.args[1];
        usize     prot    = args.args[2];
        usize     flags   = args.args[3];
        i32       fd      = args.args[4];
        off_t     offset  = args.args[5];

        Process*  process = CPU::GetCurrentThread()->parent;
        DebugSyscall(
            "MMAP: hint: {:#x}, size: {}, prot: {}, flags: {}, fd: {}, offset: "
            "{}",
            addr, len, prot, flags, fd, offset);

        if (flags & MAP_ANONYMOUS)
        {
            if (offset != 0)
            {
                LogError("MMAP: Failed, offset != 0 with MAP_ANONYMOUS");
                return_err(MAP_FAILED, EINVAL);
            }

            usize pageCount
                = Math::AlignUp(len, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;

            auto virt = addr;

            if (virt == 0) virt = VMM::AllocateSpace(len, 0, true);
            uintptr_t phys = PMM::CallocatePages<uintptr_t>(pageCount);
            process->PageMap->MapRange(virt, phys, len,
                                       PageAttributes::eRWXU
                                           | PageAttributes::eWriteBack);

            process->m_AddressSpace.push_back({phys, virt, len});
            if (0x4129659d >= virt && 0x4129659d <= virt + len)
                LogFatal("Memory in range: {:#x}", virt);
            DebugSyscall("MMAP: virt: {:#x}", virt);
            return virt;
        }

        CtosUnused(addr);
        CtosUnused(prot);
        CtosUnused(fd);

        LogError("MMAP: Failed, only MAP_ANONYMOUS is implemented");
        return_err(MAP_FAILED, EINVAL);
    }

}; // namespace Syscall::MM
