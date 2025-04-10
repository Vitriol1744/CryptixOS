/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>

INode::INode(INode* parent, std::string_view name, Filesystem* fs)
    : m_Parent(parent)
    , m_Name(name)
    , m_Filesystem(fs)
{
    Thread*  thread  = CPU::GetCurrentThread();
    Process* process = thread ? thread->GetParent() : nullptr;

    if (parent && parent->m_Stats.st_mode & S_ISUID)
    {
        m_Stats.st_uid = parent->m_Stats.st_uid;
        m_Stats.st_gid = parent->m_Stats.st_gid;

        return;
    }

    if (!process) return;

    m_Stats.st_uid = process->m_Credentials.euid;
    m_Stats.st_gid = process->m_Credentials.egid;
}

std::string INode::GetPath()
{
    std::string ret("");

    auto        current = this;
    auto        root    = VFS::GetRootNode();

    while (current && current != root)
    {
        ret.insert(0, "/" + current->m_Name);
        current = current->m_Parent;
    }

    if (ret.empty()) ret += "/";
    return ret;
}

mode_t INode::GetMode() const { return m_Stats.st_mode & ~S_IFMT; }

bool   INode::IsEmpty()
{
    m_Filesystem->Populate(this);
    return m_Children.empty();
}
bool INode::CanWrite(const Credentials& creds) const
{
    if (creds.euid == 0 || m_Stats.st_mode & S_IWOTH) return true;
    if (creds.euid == m_Stats.st_uid && m_Stats.st_mode & S_IWUSR) return true;

    return m_Stats.st_gid == creds.egid && m_Stats.st_mode & S_IWGRP;
}

bool INode::ValidatePermissions(const Credentials& creds, u32 acc)
{
    return true;
}

INode* INode::Reduce(bool symlinks, bool automount, usize cnt)
{
    if (mountGate && automount)
        return mountGate->Reduce(symlinks, automount, 0);

    if (!target.empty() && symlinks)
    {
        if (cnt >= SYMLOOP_MAX - 1) return_err(nullptr, ELOOP);

        auto nextNode
            = std::get<1>(VFS::ResolvePath(m_Parent, target, automount));
        if (!nextNode) return_err(nullptr, ENOENT);

        return nextNode->Reduce(symlinks, automount, ++cnt);
    }

    return this;
}
