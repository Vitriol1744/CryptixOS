/*
 * Created by v1tr10l7 on 19.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/mman.h>
#include <API/Posix/time.h>
#include <API/Time.hpp>

#include <Arch/x86_64/Drivers/Timers/RTC.hpp>

#include <Scheduler/Process.hpp>
#include <Time/Time.hpp>

namespace Syscall::Time
{
    ErrorOr<i32> SysGetTimeOfDay(Arguments& args)
    {
        timeval*  tv      = reinterpret_cast<timeval*>(args.Args[0]);
        timezone* tz      = reinterpret_cast<timezone*>(args.Args[1]);

        Process*  current = Process::GetCurrent();

        // TODO(v1tr10l7): Provide better abstraction over time management
        if (tv)
        {
            if (!current->ValidateAddress(tv, PROT_READ | PROT_WRITE))
                return Error(EFAULT);

            time_t now = 0;
#if CTOS_ARCH_X86_64
            now = RTC::CurrentTime();
#endif

            tv->tv_sec  = now;
            tv->tv_usec = 0;
        }
        if (tz)
        {
            if (!current->ValidateAddress(tz, PROT_READ | PROT_WRITE))
                return Error(EFAULT);

            tz->tz_dsttime     = 0;
            tz->tz_minuteswest = 0;
        }

        return Error(ENOSYS);
    }
    ErrorOr<i32> SysSetTimeOfDay(Arguments& args)
    {
        timeval*  tv      = reinterpret_cast<timeval*>(args.Args[0]);
        timezone* tz      = reinterpret_cast<timezone*>(args.Args[1]);

        // TODO(v1tr10l7): Check if user has sufficient permissions to set time
        // of day
        Process*  current = Process::GetCurrent();
        if (tv)
        {
            if (!current->ValidateAddress(tv, PROT_READ | PROT_WRITE))
                return Error(EFAULT);

            time_t now = 0;
#ifdef CTOS_TARGET_X86_64
            now = RTC::CurrentTime();
#endif
            if (tv->tv_sec < 0 || tv->tv_sec < now || tv->tv_usec < 0
                || tv->tv_usec < now / 1000)
                return Error(EINVAL);

            // TODO(vt1r10l7): Set the time of day
        }
        if (tz)
        {
            if (!current->ValidateAddress(tz, PROT_READ | PROT_WRITE))
                return Error(EFAULT);

            // TODO(vt1r10l7): Set the timezone
        }

        return Error(ENOSYS);
    }
} // namespace Syscall::Time

namespace API::Time
{
    ErrorOr<isize> SysClockGetTime(clockid_t clockID, timespec* res)
    {
        switch (clockID)
        {
            case CLOCK_REALTIME:
            case CLOCK_REALTIME_COARSE: *res = ::Time::GetReal(); break;
            case CLOCK_MONOTONIC:
            case CLOCK_MONOTONIC_RAW:
            case CLOCK_MONOTONIC_COARSE:
            case CLOCK_BOOTTIME: *res = ::Time::GetMonotonic(); break;
            case CLOCK_PROCESS_CPUTIME_ID:
            case CLOCK_THREAD_CPUTIME_ID:
                *res = {.tv_sec = 0, .tv_nsec = 0};
                break;

            default: return Error(ENOSYS);
        }

        return 0;
    }
}; // namespace API::Time
