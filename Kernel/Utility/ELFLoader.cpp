/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "ELFLoader.hpp"

#define ELF_BITNESS_32 1
#define ELF_BITNESS_64 2

typedef struct
{
    u32 magic_number;
    u8  bitness;
    u8  endianness;
    u8  header_version;
    u8  abi;
    u64 padding;
    u16 type;
    u16 instruction_set;
    u32 elf_version;
    u32 program_entry_position;
    u32 program_header_table_position;
    u32 section_header_table_position;
    u32 flags;
    u16 header_size;
    u16 program_header_size;
    u16 program_header_count;
    u16 section_header_size;
    u16 section_header_count;
    u16 section_names_header_index;
} __attribute__((packed)) elf32_header_t;

typedef struct
{
    u32 type;
    u32 offset;
    u32 virtual_address;
    u32 physical_address;
    u32 file_size;
    u32 memory_size;
    u32 flags;
    u32 alignment;
} __attribute__((packed)) elf32_program_header_t;

typedef struct
{
    u32 magic_number;
    u8  bitness;
    u8  endianness;
    u8  header_version;
    u8  abi;
    u64 padding;
    u16 type;
    u16 instruction_set;
    u32 elf_version;
    u64 program_entry_position;
    u64 program_header_table_position;
    u64 section_header_table_position;
    u32 flags;
    u16 header_size;
    u16 program_header_size;
    u16 program_header_count;
    u16 section_header_size;
    u16 section_header_count;
    u16 section_names_header_index;
} __attribute__((packed)) elf64_header_t;

typedef struct
{
    u32 type;
    u32 flags;
    u64 offset;
    u64 virtual_address;
    u64 physical_address;
    u64 file_size;
    u64 memory_size;
    u64 alignment;
} __attribute__((packed)) elf64_program_header_t;

namespace ELFLoader
{
    std::optional<std::pair<AuxVal, std::string>>
    Load(limine_file* file, PageMap* pagemap, uintptr_t base,
         uintptr_t* physAddr, uintptr_t* virtAddr)
    {
        elf64_header_t* header
            = reinterpret_cast<elf64_header_t*>(file->address);
        void* program_header_table = (u8*)&header + header->header_size;
        for (u32 i = 0; i < header->program_header_count; i++)
        {
            elf64_program_header_t* program_header
                = (elf64_program_header_t*)program_header_table
                + sizeof(elf64_program_header_t) * i;
            if (program_header->type == PT_LOAD)
            {
                u8* program = (u8*)file->address + program_header->offset;
                LogInfo("Program Size: %d bytes", program_header->memory_size);
                *physAddr = (u64)PMM::CallocatePages(
                    (u32)program_header->memory_size / PMM::PAGE_SIZE + 1);
                LogInfo("Program Physical Address: %x", *physAddr);
                memcpy((u8*)*physAddr, program,
                       (size_t)program_header->memory_size);

                *virtAddr = (u64)program_header->virtual_address;
                return {}; // program_header->memory_size;
            }
        }

        return {};
    }
}; // namespace ELFLoader
