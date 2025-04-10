/*
 * Created by v1tr10l7 on 27.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/FileDescriptor.hpp>

#include <unordered_map>

class FileDescriptorTable
{
    using TableType = std::unordered_map<i32, FileDescriptor*>;

  public:
    FileDescriptorTable() = default;

    i32         Insert(FileDescriptor* descriptor, i32 desired = -1);
    i32         Erase(i32 fdNum);

    void        OpenStdioStreams();
    void        Clear();

    inline bool IsValid(i32 fd) const
    {
        return m_Table.find(fd) != m_Table.end();
    }
    inline FileDescriptor* GetFd(i32 fd) const
    {
        if (!IsValid(fd)) return nullptr;

        return m_Table.at(fd);
    }

    TableType::iterator     begin() { return m_Table.begin(); }
    TableType::iterator     end() { return m_Table.end(); }

    inline FileDescriptor*& operator[](usize i) { return m_Table[i]; }

  private:
    Spinlock         m_Lock;
    TableType        m_Table;
    std::atomic<i32> m_NextIndex = 3;
};
