/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/sys/mman.h>
#include <API/UnixTypes.hpp>
#include <API/VFS.hpp>

#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Path.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

namespace Syscall::VFS
{
    using Syscall::Arguments;
    using namespace ::VFS;

    ErrorOr<isize> SysRead(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        void*    buffer  = reinterpret_cast<void*>(args.Args[1]);
        usize    bytes   = static_cast<usize>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        if (!buffer
            || !current->ValidateAddress(buffer, PROT_READ | PROT_WRITE))
            return std::errno_t(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        return fd->Read(buffer, bytes);
    }
    ErrorOr<isize> SysWrite(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        void*    data    = reinterpret_cast<void*>(args.Args[1]);
        usize    bytes   = static_cast<usize>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        if (!data || !current->ValidateAddress(data, PROT_READ | PROT_WRITE))
            return std::errno_t(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        return fd->Write(data, bytes);
    }

    ErrorOr<i32> SysOpen(Arguments& args)
    {
        Process*  current = CPU::GetCurrentThread()->parent;

        uintptr_t path    = args.Args[0];
        i32       flags   = static_cast<i32>(args.Args[1]);
        mode_t    mode    = static_cast<mode_t>(args.Args[2]);

        if (!path || !current->ValidateAddress(path, PROT_READ))
            return std::errno_t(EFAULT);

        return current->OpenAt(AT_FDCWD, reinterpret_cast<const char*>(path),
                               flags, mode);
    }
    ErrorOr<i32> SysClose(Arguments& args)
    {
        Process* current = CPU::GetCurrentThread()->parent;

        i32      fdNum   = static_cast<i32>(args.Args[0]);
        return current->CloseFd(fdNum);
    }
    ErrorOr<i32> SysStat(Arguments& args)
    {
        PathView path    = args.Get<const char*>(0);
        stat*    out     = args.Get<stat*>(1);

        Process* process = Process::GetCurrent();
        if (!process->ValidateAddress(reinterpret_cast<uintptr_t>(path.data()),
                                      PROT_READ)
            || !process->ValidateAddress(out, PROT_READ | PROT_WRITE))
            return Error(EFAULT);

        auto stats = VFS::Stat(AT_FDCWD, path, 0);
        if (!stats) return stats.error();
        *out = *stats.value();

        return 0;
    }
    ErrorOr<i32> SysFStat(Arguments& args)
    {
        i32      fd      = args.Get<i32>(0);
        stat*    out     = args.Get<stat*>(1);

        Process* process = Process::GetCurrent();
        if (!process->ValidateAddress(out, PROT_READ | PROT_WRITE))
            return Error(EFAULT);

        auto stats = VFS::Stat(fd, "", AT_EMPTY_PATH);
        if (!stats) return stats.error();
        *out = *stats.value();

        return 0;
    }
    ErrorOr<i32> SysLStat(Arguments& args)
    {
        PathView path    = args.Get<const char*>(0);
        stat*    out     = args.Get<stat*>(1);

        Process* process = Process::GetCurrent();
        if (!process->ValidateAddress(reinterpret_cast<uintptr_t>(path.data()),
                                      PROT_READ)
            || !process->ValidateAddress(out, PROT_READ | PROT_WRITE))
            return Error(EFAULT);

        auto stats = VFS::Stat(AT_FDCWD, path, AT_SYMLINK_NOFOLLOW);
        if (!stats) return stats.error();
        *out = *stats.value();

        return 0;
    }

    ErrorOr<off_t> SysLSeek(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        off_t    offset  = static_cast<off_t>(args.Args[1]);
        i32      whence  = static_cast<i32>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        auto     fd      = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(ENOENT);

        return fd->Seek(whence, offset);
    }

    ErrorOr<i32> SysIoCtl(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        usize    request = static_cast<usize>(args.Args[1]);
        usize    arg     = static_cast<usize>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        auto     fd      = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        return fd->GetNode()->IoCtl(request, arg);
    }
    ErrorOr<i32> SysAccess(Arguments& args)
    {
        const char* path = reinterpret_cast<const char*>(args.Args[0]);
        i32         mode = static_cast<i32>(args.Args[1]);
        (void)mode;

        INode* node = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), path));
        if (!node) return -1;

        return 0;
    }

    ErrorOr<i32> SysDup(Arguments& args)
    {
        i32      oldFd   = static_cast<i32>(args.Args[0]);

        Process* current = CPU::GetCurrentThread()->parent;

        return current->DupFd(oldFd, -1);
    }
    ErrorOr<i32> SysDup2(Syscall::Arguments& args)
    {
        i32      oldFd   = static_cast<i32>(args.Args[0]);
        i32      newFd   = static_cast<i32>(args.Args[1]);

        Process* current = CPU::GetCurrentThread()->parent;

        return current->DupFd(oldFd, newFd);
    }
    ErrorOr<i32> SysFcntl(Arguments& args)
    {
        i32             fdNum   = static_cast<i32>(args.Args[0]);
        i32             op      = static_cast<i32>(args.Args[1]);
        uintptr_t       arg     = reinterpret_cast<uintptr_t>(args.Args[2]);

        Process*        current = CPU::GetCurrentThread()->parent;
        FileDescriptor* fd      = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        switch (op)
        {
            case F_DUPFD: return current->DupFd(fdNum, arg);
            case F_DUPFD_CLOEXEC: return current->DupFd(fdNum, arg, O_CLOEXEC);
            case F_GETFD: return fd->GetFlags();
            case F_SETFD:
                if (!fd) return std::errno_t(EBADF);
                fd->SetFlags(arg);
                break;
            case F_GETFL: return fd->GetDescriptionFlags();
            case F_SETFL:
                if (arg & O_ACCMODE) return std::errno_t(EINVAL);
                fd->SetDescriptionFlags(arg);
                break;

            default:
                LogError("Syscall::VFS::SysFcntl: Unknown opcode");
                return std::errno_t(EINVAL);
        }

        return 0;
    }
    ErrorOr<i32> SysGetCwd(Arguments& args)
    {
        class Process*   current = CPU::GetCurrentThread()->parent;

        char*            buffer  = reinterpret_cast<char*>(args.Args[0]);
        usize            size    = args.Args[1];

        std::string_view cwdPath = current->GetCWD()->GetPath();
        usize            len     = std::min(size, cwdPath.length());
        cwdPath.copy(buffer, len);

        return 0;
    }
    ErrorOr<i32> SysChDir(Arguments& args)
    {
        const char* path    = reinterpret_cast<const char*>(args.Args[0]);
        Process*    current = CPU::GetCurrentThread()->parent;

        INode* node = std::get<1>(VFS::ResolvePath(current->GetCWD(), path));
        if (!node) return std::errno_t(ENOENT);
        if (!node->IsDirectory()) return std::errno_t(ENOTDIR);

        current->m_CWD = node;
        return 0;
    }
    ErrorOr<i32> SysFChDir(Arguments& args)
    {
        i32             fdNum   = static_cast<i32>(args.Args[0]);
        Process*        current = CPU::GetCurrentThread()->parent;

        FileDescriptor* fd      = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        INode* node = fd->GetNode();
        if (!node) return std::errno_t(ENOENT);

        if (!node->IsDirectory()) return std::errno_t(ENOTDIR);
        current->m_CWD = node;

        return 0;
    }

    [[clang::no_sanitize("alignment")]]
    ErrorOr<i32> SysGetDents64(Arguments& args)
    {
        u32           fdNum     = static_cast<u32>(args.Args[0]);
        dirent* const outBuffer = reinterpret_cast<dirent* const>(args.Args[1]);
        u32           count     = static_cast<u32>(args.Args[2]);

        Process*      current   = Process::GetCurrent();
        if (!outBuffer
            || !current->ValidateAddress(outBuffer, PROT_READ | PROT_WRITE))
            return std::errno_t(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        return fd->GetDirEntries(outBuffer, count);
    }

    ErrorOr<i32> SysOpenAt(Arguments& args)
    {
        Process* current = CPU::GetCurrentThread()->parent;

        i32      dirFd   = static_cast<i32>(args.Args[0]);
        PathView path    = reinterpret_cast<const char*>(args.Args[1]);
        i32      flags   = static_cast<i32>(args.Args[2]);
        mode_t   mode    = static_cast<mode_t>(args.Args[3]);

        return current->OpenAt(dirFd, path, flags, mode);
    }
    ErrorOr<i32> SysFStatAt(Arguments& args)
    {
        i32         fdNum   = args.Get<i32>(0);
        const char* path    = args.Get<const char*>(1);
        i32         flags   = args.Get<i32>(2);
        stat*       out     = args.Get<stat*>(3);

        Process*    process = CPU::GetCurrentThread()->parent;
        if (!process->ValidateAddress(reinterpret_cast<uintptr_t>(path),
                                      PROT_READ)
            || !process->ValidateAddress(out, PROT_READ | PROT_WRITE))
            return Error(EFAULT);

        if (!out) return std::errno_t(EFAULT);
        auto stats = VFS::Stat(fdNum, path, flags);
        if (!stats) return stats.error();
        *out = *stats.value();

        return 0;
    }

    ErrorOr<i32> SysDup3(Arguments& args)
    {
        i32 oldFdNum = static_cast<i32>(args.Args[0]);
        i32 newFdNum = static_cast<i32>(args.Args[1]);
        i32 flags    = static_cast<i32>(args.Args[2]);

        if (oldFdNum == newFdNum) return std::errno_t(EINVAL);
        if ((flags & ~O_CLOEXEC) != 0) return std::errno_t(EINVAL);

        Process* current = CPU::GetCurrentThread()->parent;
        return current->DupFd(oldFdNum, newFdNum, flags);
    }
}; // namespace Syscall::VFS
