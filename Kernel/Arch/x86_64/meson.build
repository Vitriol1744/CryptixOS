#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'Drivers/IoApic.cpp',
  'Drivers/PIC.cpp',
  'Drivers/Serial.cpp',
  'Drivers/Timers/Lapic.cpp',
  'Drivers/Timers/PIT.cpp',
  'Drivers/Timers/RTC.cpp',
  'Arch.cpp',
  'CMOS.cpp',
  'CPU.cpp',
  'GDT.cpp',
  'IDT.cpp',
  'InterruptManager.cpp',

  'VMM.cpp',
  'interrupts.x86_64.S',
)

cxx_args += [
  '-m64',
  '-march=x86-64',
  '-msoft-float',
  #'-mno-80387',
  '-mno-mmx',
  '-mno-sse',
  '-mno-sse2',
  '-mno-red-zone',
  '-mcmodel=kernel',
]

ld_args += [
  '-Wl,-z,noexecstack',
  '-Wl,-m,elf_x86_64',
]
