/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <VFS/INode.hpp>

class DevTmpFsINode : public INode, NonCopyable<DevTmpFsINode>
{
  public:
    DevTmpFsINode(INode* parent, std::string_view name, Filesystem* fs,
                  mode_t mode, Device* device = nullptr);
    virtual ~DevTmpFsINode()
    {
        if (m_Capacity > 0) delete m_Data;
    }

    virtual const stat& GetStats() override
    {
        return m_Device ? m_Device->GetStats() : m_Stats;
    }

    virtual void InsertChild(INode* node, std::string_view name) override
    {
        ScopedLock guard(m_Lock);
        m_Children[name] = node;
    }

    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual i32   IoCtl(usize request, usize arg) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    virtual ErrorOr<void>  ChMod(mode_t mode) override;

  private:
    Device* m_Device   = nullptr;
    u8*     m_Data     = nullptr;
    usize   m_Capacity = 0;
};
