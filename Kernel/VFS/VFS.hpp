/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Utility/Path.hpp"
#include "Utility/UnixTypes.hpp"

class INode;
enum class INodeType;

namespace VFS
{
    INode* GetRootNode();
    void   RecursiveDelete(INode* node);

    std::tuple<INode*, INode*, std::string>
    ResolvePath(INode* parent, std::string_view path, bool automount = true);

    bool   MountRoot(std::string_view filesystemName);
    bool   Mount(INode* parent, PathView source, PathView target,
                 std::string_view fsName, i32 flags = 0, void* data = nullptr);
    bool   Unmount(INode* parent, PathView path, i32 flags = 0);

    INode* CreateNode(INode* parent, PathView path, mode_t mode,
                      INodeType type);
    INode* Symlink(INode* parent, PathView path, std::string_view target);
    INode* Link(INode* oldParent, PathView oldPath, INode* newParent,
                PathView newPath, i32 flags = 0);
    bool   Unlink(INode* parent, PathView path, i32 flags = 0);
}; // namespace VFS