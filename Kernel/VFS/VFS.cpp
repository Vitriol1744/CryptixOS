
/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "VFS.hpp"

#include "VFS/INode.hpp"
#include "VFS/TmpFs/TmpFs.hpp"

#include <cerrno>

namespace VFS
{
    static INode*      rootINode = nullptr;
    static std::mutex  lock;

    INode*             GetRootNode() { return rootINode; }

    static Filesystem* CreateFilesystem(std::string_view name)
    {
        Filesystem* fs = nullptr;
        if (name == "tmpfs") fs = new TmpFs();

        return fs;
    }

    void RecursiveDelete(INode* node)
    {
        if (!node) return;

        if (node->GetType() == INodeType::eDirectory)
            for (auto [name, child] : node->GetChildren())
                RecursiveDelete(child);

        delete node;
    }

    std::tuple<INode*, INode*, std::string>
    ResolvePath(INode* parent, std::string_view path, bool automount)
    {
        if (!parent || Path::IsAbsolute(path)) parent = GetRootNode();

        auto currentNode = parent->Reduce(false);

        if (path == "/" || path.empty()) return {currentNode, currentNode, "/"};

        auto getParent = [&currentNode]
        {
            if (currentNode == GetRootNode()->Reduce(false)) return currentNode;
            else if (currentNode == currentNode->GetFilesystem()->GetRootNode())
                return currentNode->GetFilesystem()
                    ->GetMountedOn()
                    ->GetParent();

            return currentNode->GetParent();
        };

        auto segments = Path::SplitPath(path.data());

        for (size_t i = 0; i < segments.size(); i++)
        {
            auto segment     = segments[i];
            bool isLast      = i == (segments.size() - 1);

            bool currentDir  = segment.starts_with(".");
            bool previousDir = segment.starts_with("..");

            if (currentDir || previousDir)
            {
                if (previousDir) currentNode = getParent();

                if (isLast)
                    return {getParent(), currentNode, currentNode->GetName()};

                continue;
            }

            if (currentNode->GetChildren().contains(segment)
                || currentNode->GetFilesystem()->Populate(currentNode))
            {
                auto node = currentNode->GetChildren()[segment]->Reduce(
                    false, !isLast || automount);

                if (isLast) return {currentNode, node, node->GetName()};

                currentNode = node;

                if (currentNode->GetType() == INodeType::eHardLink)
                {
                    currentNode = currentNode->Reduce(true);
                    if (!currentNode) return {nullptr, nullptr, ""};
                }

                if (currentNode->GetType() != INodeType::eDirectory)
                {
                    errno = ENOTDIR;
                    return {nullptr, nullptr, ""};
                }

                continue;
            }

            errno = ENOENT;
            if (isLast)
                return {
                    currentNode, nullptr, {segment.data(), segment.length()}};

            break;
        }

        errno = ENOENT;
        return {nullptr, nullptr, ""};
    }

    bool MountRoot(std::string_view filesystemName)
    {
        auto fs = CreateFilesystem(filesystemName);
        if (!fs)
        {
            LogError("Failed to create filesystem!");
            return false;
        }
        if (rootINode)
        {
            LogError("VFS: Root already mounted!");
            return false;
        }

        rootINode = fs->Mount(nullptr, nullptr, nullptr, "/", nullptr);

        return rootINode != nullptr;
    }
    // TODO: flags
    bool Mount(INode* parent, PathView source, PathView target,
               std::string_view fsName, int flags, void* data)
    {
        std::unique_lock guard(lock);

        Filesystem*      fs = CreateFilesystem(fsName);
        if (fs == nullptr)
        {
            errno = ENODEV;
            return false;
        }
        [[maybe_unused]] INode* sourceNode = nullptr;

        auto [nparent, node, basename]     = ResolvePath(parent, target);
        bool isRoot                        = (node == GetRootNode());

        if (!node) return false;

        if (!isRoot && node->GetType() != INodeType::eDirectory)
        {
            errno = ENOTDIR;
            return false;
        }

        auto mountGate = fs->Mount(parent, nullptr, node, basename, data);
        if (!mountGate) return false;

        node->mountGate = mountGate;

        if (source.length() == 0)
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName.data(),
                     target.data());
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     source.data(), target.data(), fsName.data());
        return true;
    }

    bool Unmount(INode* parent, PathView path, int flags)
    {
        // TODO: Unmount
        ToDo();
        return false;
    }

    INode* CreateNode(INode* parent, PathView path, mode_t mode, INodeType type)
    {
        std::unique_lock guard(lock);

        auto [newNodeParent, newNode, newNodeName] = ResolvePath(parent, path);
        if (newNode)
        {
            errno = EEXIST;
            return nullptr;
        }

        if (!newNodeParent) return nullptr;

        newNode = newNodeParent->GetFilesystem()->CreateNode(
            newNodeParent, newNodeName, mode, type);
        if (newNode) newNodeParent->InsertChild(newNode, newNode->GetName());
        return newNode;
    }

    INode* Symlink(INode* parent, PathView path, std::string_view target)
    {
        std::unique_lock guard(lock);

        auto [newNodeParent, newNode, newNodeName] = ResolvePath(parent, path);
        if (newNode)
        {
            errno = EEXIST;
            return nullptr;
        }

        if (!newNodeParent) return nullptr;

        newNode = newNodeParent->GetFilesystem()->Symlink(newNodeParent,
                                                          newNodeName, target);
        if (newNode) newNodeParent->InsertChild(newNode, newNode->GetName());
        return newNode;
    }

    INode* Link(INode* oldParent, PathView oldPath, INode* newParent,
                PathView newPath, int flags)
    {
        return nullptr;
    }

    bool Unlink(INode* parent, PathView path, int flags) { return false; }
} // namespace VFS