#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'ACPI/ACPI.cpp',
  'ACPI/MADT.cpp',

  'Drivers/Terminal.cpp',
  'Memory/KernelHeap.cpp',

  'Memory/PhysicalMemoryManager.cpp',
  'Memory/VirtualMemoryManager.cpp',

  'Scheduler/Scheduler.cpp',
  'Scheduler/Thread.cpp',

  'Utility/BootInfo.cpp',
  'Utility/ELFLoader.cpp',
  'Utility/Logger.cpp',
  'Utility/Stacktrace.cpp',

  'KernelStart.cpp',
)

cxx_args = []

ld_args = [
  '-nostdlib',
  '-static',
  '-fuse-ld=lld',
  '-Wl,--no-dynamic-linker',
  '-Wl,-z,text',
  #'-pie',
  '-Wl,-z,max-page-size=0x1000',
  '-Wl,-T,' + meson.current_source_dir() + '/linker.' + arch + '.ld',
]

if arch == 'x86_64'
  cxx_args += ['-DCTOS_ARCH=CTOS_ARCH_X86_64']
  subdir('Arch/x86_64')
elif arch == 'aarch64'
  subdir('Arch/aarch64')
  cxx_args += ['-DCTOS_ARCH=CTOS_ARCH_AARCH64']
else
  error('Unknown architecture: ' + arch)
endif

subdir('Utility/KLibC')
subdir('Utility/KLibC++')

args = []
kernel_elf = executable(
  'Cryptix.elf',
  srcs,
  dependencies: deps,
  include_directories: include_directories,
  cpp_args: cxx_args,
  link_args: ld_args,
  install: false,
)
