/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Drivers/Storage/NVMe/NVMeNameSpace.hpp>
#include <Drivers/Storage/NVMe/NVMeQueue.hpp>

#include <Memory/PMM.hpp>

namespace NVMe
{
    using Prism::Pointer;

    Queue::Queue(Pointer crAddress, u16 qid, u32 doorbellShift, u64 depth)
        : m_ID(qid)
        , m_Depth(depth)
    {
        auto submitDbOffset
            = PMM::PAGE_SIZE + (m_ID * 2) * (4 << doorbellShift);
        auto completeDbOffset
            = PMM::PAGE_SIZE + (m_ID * 2 + 1) * (4 << doorbellShift);

        m_Submit = new volatile Submission[depth];
        m_SubmitDoorbell
            = crAddress.Offset<Pointer>(submitDbOffset).As<volatile u32>();

        m_SubmitHead = 0;
        m_SubmitTail = 0;

        m_Complete   = new volatile Completion[depth];

        m_CompleteDoorbell
            = crAddress.Offset<Pointer>(completeDbOffset).As<volatile u32>();
        m_CompleteVec   = 0;
        m_CompleteHead  = 0;
        m_CompletePhase = 1;

        m_CmdId         = 0;
        m_PhysRegPgs    = nullptr;
    }
    Queue::Queue(Pointer crAddress, NameSpace& ns, u16 qid, u32 doorbellShift,
                 u64 depth)
        : Queue(crAddress, qid, doorbellShift, depth)
    {
        m_PhysRegPgs = new u64[ns.GetMaxPhysRPgs() * depth];
    }

    u16 Queue::AwaitSubmit(Submission* cmd)
    {
        u16 currentHead  = m_CompleteHead;
        u16 currentPhase = m_CompletePhase;
        cmd->CompleteID  = m_CmdId++;
        Submit(cmd);
        u16 status = 0;

        while (true)
        {
            status = m_Complete[m_CompleteHead].Status;
            if ((status & 0x01) == currentPhase) break;
        }

        status >>= 1;
        AssertFmt(!status, "NVMe: Command error: {:#x}", status);

        currentHead = (currentHead + 1) % m_Depth;
        if (currentHead == 0) currentPhase = !currentPhase;

        *(m_CompleteDoorbell) = currentHead;
        m_CompleteHead        = currentHead;
        m_CompletePhase       = currentPhase;
        return status;
    }

    void Queue::Submit(Submission* cmd)
    {
        u16       currentTail = m_SubmitTail;

        uintptr_t dest = reinterpret_cast<uintptr_t>(&m_Submit[currentTail]);

        std::memcpy(reinterpret_cast<u8*>(dest), cmd, sizeof(Submission));
        currentTail         = (currentTail + 1) % m_Depth;

        *(m_SubmitDoorbell) = currentTail;
        m_SubmitTail        = currentTail;
    }
}; // namespace NVMe
