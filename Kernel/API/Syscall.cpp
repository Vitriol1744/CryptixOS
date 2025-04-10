/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/SyscallEntryPoints.hpp>

#include <API/MM.hpp>
#include <API/Process.hpp>
#include <API/System.hpp>
#include <API/Time.hpp>
#include <API/VFS.hpp>

#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <array>

namespace Syscall
{
    void Initialize();

    struct Syscall
    {
        std::string                                   name;
        std::function<ErrorOr<uintptr_t>(Arguments&)> handler;

        inline ErrorOr<uintptr_t> operator()(Arguments& args)
        {
            if (handler.operator bool()) return handler(args);

            return 0;
        }
        inline operator bool() { return handler.operator bool(); }
    };
    static std::array<Syscall, 512>      syscalls;

    std::unordered_map<ID, WrapperBase*> s_Syscalls;

    void
    RegisterHandler(usize                                              index,
                    std::function<ErrorOr<uintptr_t>(Arguments& args)> handler,
                    std::string                                        name)
    {
        syscalls[index] = {name, handler};
    }
#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004
    static uintptr_t SysArchPrCtl(Arguments& args)
    {
#ifdef CTOS_TARGET_X86_64
        auto      thread = CPU::GetCurrentThread();
        i32       op     = args.Args[0];
        uintptr_t addr   = args.Args[1];

        switch (op)
        {
            case ARCH_SET_GS:
                thread->SetGsBase(addr);
                CPU::SetKernelGSBase(thread->GetGsBase());
                break;
            case ARCH_SET_FS:
                thread->SetFsBase(addr);
                CPU::SetFSBase(thread->GetFsBase());
                break;
            case ARCH_GET_FS:
                *reinterpret_cast<uintptr_t*>(addr) = thread->GetFsBase();
                break;
            case ARCH_GET_GS:
                *reinterpret_cast<uintptr_t*>(addr) = thread->GetGsBase();
                break;

            default: return_err(-1, EINVAL);
        }
#endif

        return 0;
    }

    void InstallAll()
    {
        Initialize();

        RegisterSyscall2(ID::eRead, API::VFS::Read);
        RegisterSyscall2(ID::eWrite, API::VFS::Write);
        RegisterSyscall2(ID::eOpen, API::VFS::Open);
        RegisterSyscall2(ID::eClose, API::VFS::Close);
        RegisterSyscall2(ID::eStat, API::VFS::Stat);
        RegisterSyscall2(ID::eFStat, API::VFS::FStat);
        RegisterSyscall2(ID::eLStat, API::VFS::LStat);
        RegisterSyscall(ID::eLSeek, VFS::SysLSeek);
        RegisterSyscall(ID::eMMap, SysMMap);
        RegisterSyscall(ID::eMProtect, MM::SysMProtect);
        RegisterSyscall(ID::eMUnMap, MM::SysMUnMap);
        RegisterSyscall2(ID::eSigProcMask, API::Process::SigProcMask);
        RegisterSyscall(ID::eIoCtl, VFS::SysIoCtl);
        RegisterSyscall(ID::eAccess, VFS::SysAccess);
        RegisterSyscall(ID::ePipe, SysPipe);
        RegisterSyscall2(ID::eSchedYield, API::Process::SchedYield);
        RegisterSyscall2(ID::eDup, API::VFS::Dup);
        RegisterSyscall2(ID::eDup2, API::VFS::Dup2);
        RegisterSyscall(ID::eNanoSleep, SysNanoSleep);
        RegisterSyscall2(ID::eGetPid, API::Process::GetPid);
        RegisterSyscall(ID::eExit, Process::SysExit);
        RegisterSyscall(ID::eWait4, Process::SysWait4);
        RegisterSyscall(ID::eKill, Process::SysKill);
        RegisterSyscall2(ID::eGetUid, API::Process::GetUid);
        RegisterSyscall2(ID::eGetGid, API::Process::GetGid);
        RegisterSyscall2(ID::eUname, API::System::Uname);
        RegisterSyscall2(ID::eGetResourceLimit, API::System::GetResourceLimit);
        RegisterSyscall2(ID::eGetResourceUsage, API::System::GetResourceUsage);
        RegisterSyscall(ID::eFcntl, VFS::SysFcntl);
        RegisterSyscall2(ID::eTruncate, API::VFS::Truncate);
        RegisterSyscall2(ID::eFTruncate, API::VFS::FTruncate);
        RegisterSyscall2(ID::eGetCwd, API::VFS::GetCwd);
        RegisterSyscall(ID::eChDir, VFS::SysChDir);
        RegisterSyscall(ID::eFChDir, VFS::SysFChDir);
        RegisterSyscall(ID::eMkDir, VFS::SysMkDir);
        RegisterSyscall(ID::eRmDir, SysRmDir);
        RegisterSyscall(ID::eCreat, SysCreat);
        RegisterSyscall(ID::eReadLink, SysReadLink);
        RegisterSyscall2(ID::eChMod, API::VFS::ChMod);
        RegisterSyscall2(ID::eUmask, API::Process::Umask);
        RegisterSyscall(ID::eGetTimeOfDay, Time::SysGetTimeOfDay);
        RegisterSyscall2(ID::eGet_eUid, API::Process::GetEUid);
        RegisterSyscall2(ID::eGet_eGid, API::Process::GetEGid);
        RegisterSyscall(ID::eSet_pGid, Process::SysSet_pGid);
        RegisterSyscall2(ID::eGet_pPid, API::Process::GetPPid);
        RegisterSyscall(ID::eGetPgrp, Process::SysGetPgrp);
        RegisterSyscall(ID::eSetSid, Process::SysSetSid);
        RegisterSyscall(ID::eGet_pGid, Process::SysGet_pGid);
        RegisterSyscall(ID::eGetSid, Process::SysGetSid);
        RegisterSyscall2(ID::eUTime, API::VFS::UTime);
        RegisterSyscall(ID::eFork, Process::SysFork);
        RegisterSyscall(ID::eExecve, Process::SysExecve);
        RegisterSyscall(ID::eArchPrCtl, SysArchPrCtl);
        RegisterSyscall(ID::eSetTimeOfDay, Time::SysSetTimeOfDay);
        RegisterSyscall2(ID::eMount, API::VFS::Mount);
        RegisterSyscall2(ID::eReboot, API::System::Reboot);
        // RegisterSyscall(ID::eGetTid, Process::SysGetTid);
        RegisterSyscall(ID::eGetDents64, VFS::SysGetDents64);
        RegisterSyscall(ID::eClockGetTime, SysClockGetTime);
        RegisterSyscall(ID::eNanoSleep, Process::SysNanoSleep);
        RegisterSyscall2(ID::ePanic, API::System::SysPanic);
        RegisterSyscall(ID::eOpenAt, VFS::SysOpenAt);
        RegisterSyscall(ID::eMkDirAt, VFS::SysMkDirAt);
        RegisterSyscall2(ID::eFStatAt, API::VFS::FStatAt);
        RegisterSyscall2(ID::eFChModAt, API::VFS::FChModAt);
        RegisterSyscall2(ID::eDup3, API::VFS::Dup3);
    }
    void Handle(Arguments& args)
    {
#define LOG_SYSCALLS false
#if LOG_SYSCALLS == true
        static isize previousSyscall = -1;

        if (static_cast<isize>(args.Index) != previousSyscall)
            LogTrace(
                "Syscall[{}]: '{}'\nparams: {{ arg[0]: {}, arg[1]: {}, "
                "arg[2]: {}, arg[3]: {}, arg[4]: {}, arg[5]: {}, }}",
                args.Index, magic_enum::enum_name(static_cast<ID>(args.Index)),
                args.Get<u64>(0), args.Get<u64>(1), args.Get<u64>(2),
                args.Get<u64>(3), args.Get<u64>(4), args.Get<u64>(5));

        previousSyscall = args.Index;
#endif

        if (args.Index >= 512
            || (!syscalls[args.Index]
                && !s_Syscalls.contains(static_cast<ID>(args.Index))))
        {
            args.ReturnValue = -1;
            errno            = ENOSYS;
            LogError(
                "Undefined syscall: {}\nparams: {{ arg[0]: {}, arg[1]: {}, "
                "arg[2]: {}, arg[3]: {}, arg[4]: {}, arg[5]: {}, }}",
                args.Index, args.Get<u64>(0), args.Get<u64>(1),
                args.Get<u64>(2), args.Get<u64>(3), args.Get<u64>(4),
                args.Get<u64>(5));

            return;
        }

        errno = no_error;
        std::array<uintptr_t, 6> arr
            = {args.Args[0], args.Args[1], args.Args[2],
               args.Args[3], args.Args[4], args.Args[5]};
#define SYSCALL_LOG_ERR false
#if SYSCALL_LOG_ERR == true
    #define SyscallError(...) LogError(__VA_ARGS__)
#else
    #define SyscallError(...)
#endif

        if (s_Syscalls.contains(static_cast<ID>(args.Index)))
        {
            auto ret = s_Syscalls[static_cast<ID>(args.Index)]->Run(arr);

            if (ret) args.ReturnValue = ret.value();
            else
            {
                SyscallError(
                    "Syscall: '{}' caused error",
                    magic_enum::enum_name(static_cast<ID>(args.Index)).data()
                        + 1);
                args.ReturnValue = -intptr_t(ret.error());
            }
            return;
        }

        auto ret = syscalls[args.Index](args);
        if (ret) args.ReturnValue = ret.value();
        else
        {
            SyscallError(
                "Syscall: '{}' caused error",
                magic_enum::enum_name(static_cast<ID>(args.Index)).data() + 1);
            args.ReturnValue = -intptr_t(ret.error());
        }
    }
} // namespace Syscall
