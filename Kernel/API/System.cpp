/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/utsname.h>
#include <API/System.hpp>

#include <Arch/Arch.hpp>
#include <Arch/PowerManager.hpp>

namespace Syscall::System
{
    ErrorOr<isize> SysUname(Arguments& args)
    {
        utsname* out = reinterpret_cast<utsname*>(args.Args[0]);
        strncpy(out->sysname, "Cryptix", sizeof(out->sysname));
        strncpy(out->nodename, "cryptix", sizeof(out->sysname));
        strncpy(out->release, "0.0.1", sizeof(out->sysname));
        strncpy(out->version, __DATE__ " " __TIME__, sizeof(out->sysname));
        strncpy(out->machine, CTOS_ARCH_STRING, sizeof(out->sysname));

        return 0;
    }
    ErrorOr<i32> SysReboot(RebootCommand cmd)
    {
        switch (cmd)
        {
            case RebootCommand::eRestart: PowerManager::Reboot(); break;

            default: Prism::Log::Warn("SysReboot: Unknown OpCode!"); break;
        }

        return Error(ENOSYS);
    }
} // namespace Syscall::System
