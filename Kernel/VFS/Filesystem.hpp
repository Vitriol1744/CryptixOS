/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "API/UnixTypes.hpp"
#include "Utility/Path.hpp"

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>

class INode;
class Filesystem
{
  public:
    Filesystem(const std::string& name, u32 flags)
        : mountedOn(nullptr)
        , mountData(nullptr)
        , name(name)
    {
    }

    inline INode*  GetMountedOn() { return mountedOn; }
    inline INode*  GetRootNode() { return root; }
    inline ino_t   GetNextINodeIndex() { return nextInodeIndex++; }
    inline dev_t   GetDeviceID() const { return deviceID; }

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name, void* data = nullptr)
        = 0;
    virtual INode* CreateNode(INode* parent, std::string_view name, mode_t mode)
        = 0;
    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target)
        = 0;

    virtual INode* Link(INode* parent, std::string_view name, INode* oldNode)
        = 0;
    virtual bool   Populate(INode* node) = 0;

    virtual INode* MkNod(INode* parent, std::string_view path, mode_t mode,
                         dev_t dev)
    {
        return nullptr;
    }

  protected:
    INode*             mountedOn = nullptr;
    void*              mountData = nullptr;
    INode*             root      = nullptr;

    std::mutex         lock;

    std::string        name;
    std::atomic<ino_t> nextInodeIndex = 0;
    dev_t              deviceID       = 0;
};
