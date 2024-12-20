#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
project(
  'CryptixOS',
  ['c', 'cpp'],
  version: 'v0.0.1',
  license: 'GPL-3.0',
  default_options: [
    'c_std=c17',
    'warning_level=2',
    'werror=true',
  ],
)

c = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
ld = cxx.get_linker_id()
arch = host_machine.cpu_family()
qemulog = meson.source_root() / 'qemu.log'

srcs = []
initrd_dir = meson.source_root() / 'BaseFiles'

include_directories = [
  include_directories('Kernel'),
]

# ==============================================================================
### Compiler arguments
warnings = [
  '-Wno-error=#warnings',
  '-Wno-builtin-macro-redefined',
  '-Wno-macro-redefined',
  '-Wno-nan-infinity-disabled',
]

add_global_arguments(
  [
    warnings,
    '-ffreestanding',
    '-fno-stack-protector',
    '-fno-stack-check',
    '-fno-omit-frame-pointer',
    '-fno-lto',
    #'-fPIE',
    '-m64',
    '-fno-pic',
    '-fno-pie',
    '-Ofast',
  ],
  language: ['c', 'cpp'],
)

if not meson.get_compiler('cpp').has_argument('-std=gnu++23')
  error('C++23 support is needed. Please install clang 17 or higher.')
endif

add_global_arguments(
  [
    '-std=gnu++23',
    '-fno-rtti',
    '-fno-exceptions',
    '-fsized-deallocation',
    '-Wno-unused-parameter',
    '-Wno-non-virtual-dtor',
    '-Wno-frame-address',
    '-D__cplusplus=202302L',
    '-D__cpp_concepts=202207L',
    '-DFMT_STATIC_THOUSANDS_SEPARATOR=1',
    '-DFMT_USE_LONG_DOUBLE=0',
    '-DFMT_USE_DOUBLE=0',
    '-DFMT_USE_FLOAT=0',
    '-MP',
  ],
  language: 'cpp',
)

# ==============================================================================
### Dependencies
cmake = import('cmake')
phmap_opts = cmake.subproject_options()
phmap_opts.add_cmake_defines(
  {
    'PHMAP_BUILD_EXAMPLES': false,
    'PHMAP_BUILD_TESTS': false,
    'PHMAP_INSTALL': false,
  },
)

limine = subproject('limine', default_options: 'arch=' + arch)
deps = [
  cmake.subproject('frozen').dependency('frozen'),

  dependency('limine'),
  dependency('libstdcxx_headers'),
  dependency(
    'compiler_rt_builtins',
    default_options: 'custom_includes=' + meson.source_root() / 'Kernel/Utility/KLibC',
  ),
  dependency('smart_ptr'),
  dependency('veque'),
  dependency('fmt'),
  dependency('string'),
]
#cmake.subproject('parallel_hashmap', options : phmap_opts).dependency('phmap'),

inc_deps = []
foreach dep : deps
  inc_deps += dep.partial_dependency(includes: true)
endforeach

subdir('Kernel')

# ==============================================================================
### Qemu args
qemu_args = [
  '-cpu', 'max',
  '-net', 'none',
  '-serial', 'stdio',
  '-d', 'int,guest_errors',
  '-smp', '4',
  '-d', 'cpu_reset',
  '-m', '1024M',
  '-rtc', 'base=localtime',
  '-no-shutdown',
  '-no-reboot',
  '-D', meson.source_root() / 'qemu.log',
  '-boot', 'order=d,menu=on,splash-time=100',
  '-audiodev', 'id=audio,driver=alsa',
]
qemu = 'qemu-system-' + arch

if arch == 'x86_64'
  qemu_args += [
    '-M', 'q35,smm=off',
    '-device', 'isa-debug-exit',
    '-debugcon', 'file:' + meson.source_root() / 'debug.log',
    '-enable-kvm',
    '-M', 'accel=kvm:hvf:whpx:haxm:tcg',
    '-machine', 'pcspk-audiodev=audio',
    '-cpu', 'host',
  ]
  ovmf_id = 'X64'
  xorriso_args = [
    '-b', 'limine-bios-cd.bin',
    '-no-emul-boot',
    '-boot-load-size', '4',
    '-boot-info-table',
    '--efi-boot', 'limine-uefi-cd.bin',
    '-efi-boot-part',
    '--efi-boot-image',
    '--protective-msdos-label',
  ]

elif arch == 'aarch64'
  qemu_args += [
    '-M', 'virt',
    '-device', 'ramfb',
  ]
  ovmf_id = 'AA64'
  xorriso_args = [
    '--efi-boot', 'limine-uefi-cd.bin',
    '-efi-boot-part',
    '--efi-boot-image',
    '--protective-msdos-label',
  ]
else
  error('Unknown architecture: ' + arch)
endif
# ==============================================================================

iso_dir = 'iso_root'
ksyms = custom_target(
  'ksyms',
  input: [kernel_elf],
  output: 'ksyms.sym',
  command: ['./generate_symbols.sh'],
  build_by_default: true,
  console: true,
  install: false,
  depends: [kernel_elf],
)

iso_files = [
  meson.source_root() / 'limine.conf',
  ksyms,
  meson.source_root() / 'init',
  limine.get_variable('limine_binaries'),
]

initrd = custom_target(
  'initrd',
  output: 'initrd.img.gz',
  command: ['tar', '--format', 'posix', '-czf', '@OUTPUT@', '-C', initrd_dir, './'],
  build_by_default: true,
  console: true,
  install: false,
)

iso = custom_target(
  'iso',
  input: [kernel_elf, initrd],
  output: 'Cryptix.iso',
  command: [
    'mkdir',
    '-p', iso_dir,
    '&&',
    'cp',
    '@INPUT0@',
    iso_dir / 'Cryptix.elf',
    '&&',
    'cp',
    '@INPUT1@',
    iso_dir / 'initrd.img.gz',
    '&&',
    'cp',
    iso_files,
    iso_dir,
    '&&',
    'xorriso',
    '-as', 'mkisofs',
    xorriso_args,
    iso_dir,
    '-o', '@OUTPUT@',
    '&&',
    meson.source_root() / 'subprojects/limine/limine',
    'bios-install',
    '@OUTPUT@',
    '&&',
    'rm',
    '-rf', iso_dir,
  ],
  build_by_default: true,
  console: true,
  install: false,
  depends: [kernel_elf, initrd],
)

run_bios = custom_target(
  'run_bios',
  input: iso,
  output: 'run_bios',
  command: [qemu, qemu_args, '-cdrom', '@INPUT@'],
  build_by_default: true,
  console: true,
  install: false,
  depends: [iso],
)

ovmf_binaries = subproject('ovmf_binaries')
ovmf = ovmf_binaries.get_variable(ovmf_id)

run_uefi = custom_target(
  'run_uefi',
  input: iso,
  output: 'run_uefi',
  command: [qemu, qemu_args, '-cdrom', '@INPUT@', '-bios', ovmf],
  console: true,
  install: false,
  depends: iso,
)

run_uefi_gdb = custom_target(
  'run_uefi_gdb',
  input: iso,
  output: 'run_uefi_gdb',
  command: [qemu, qemu_args, '-s', '-S', '-cdrom', '@INPUT@', '-bios', ovmf],
  console: true,
  install: false,
  depends: iso,
)

onlybuild = custom_target(
  'onlybuild',
  output: 'onlybuild',
  command: 'true',
  install: false,
  depends: iso,
)
