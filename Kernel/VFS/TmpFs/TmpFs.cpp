/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Memory/PMM.hpp>

#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

TmpFs::TmpFs(u32 flags)
    : Filesystem("TmpFs", flags)
    , maxInodes(0)
    , maxSize(0)
    , currentSize(0)
{
}

INode* TmpFs::Mount(INode* parent, INode* source, INode* target,
                    std::string_view name, void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    maxSize   = PMM::GetTotalMemory() / 2;
    maxInodes = PMM::GetTotalMemory() / PMM::PAGE_SIZE / 2;

    m_Root    = CreateNode(parent, name, 0644 | S_IFDIR);
    if (m_Root) m_MountedOn = target;
    return m_Root;
}
INode* TmpFs::CreateNode(INode* parent, std::string_view name, mode_t mode)
{
    if (m_NextInodeIndex >= maxInodes
        || (S_ISREG(mode)
            && currentSize + TmpFsINode::GetDefaultSize() > maxSize))
    {
        errno = ENOSPC;

        return nullptr;
    }

    return new TmpFsINode(parent, name, this, mode);
}

INode* TmpFs::Symlink(INode* parent, std::string_view name,
                      std::string_view target)
{
    if (m_NextInodeIndex >= maxInodes) return_err(nullptr, ENOSPC);

    auto node    = new TmpFsINode(parent, name, this, 0777 | S_IFLNK);
    node->target = target;

    LogInfo("TmpFs: Created symlink '{}' -> '{}'", name, target);
    return node;
}

INode* TmpFs::Link(INode* parent, std::string_view name, INode* oldNode)
{
    if (oldNode->IsDirectory())
    {
        errno = EISDIR;
        return nullptr;
    }

    return new TmpFsINode(parent, name, this,
                          (oldNode->GetStats().st_mode & ~S_IFMT));
}
