/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Scheduler/Event.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>

class Fifo : public INode
{
  public:
    Fifo();

    enum class Direction
    {
        eRead  = 0,
        eWrite = 1,
    };

    FileDescriptor* Open(Direction direction);

    virtual void    InsertChild(INode*, std::string_view) override
    {
        AssertNotReached();
    }
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;

    virtual const stat& GetStats() override
    {
        LogTrace("Stats");
        return m_Stats;
    }

  private:
    usize m_ReaderCount = 0;
    usize m_WriterCount = 0;
    Event m_Event;

    u8*   m_Data        = nullptr;
    usize m_Size        = 0;
    usize m_Capacity    = 0;
    usize m_ReadOffset  = 0;
    usize m_WriteOffset = 0;
    bool  m_NonBlocking = false;

    void  Attach(Direction direction);
    void  Detach();
};
