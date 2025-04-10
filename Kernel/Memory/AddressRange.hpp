/*
 * Created by v1tr10l7 on 17.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

class AddressRange final
{
  public:
    AddressRange() = default;

    template <typename T>
    AddressRange(T address, usize size)
        : m_Base(address)
        , m_Size(size)
    {
    }

    inline PM::Pointer GetBase() const { return m_Base; }
    inline usize       GetSize() const { return m_Size; }

    inline             operator bool() { return m_Base.operator bool(); }

    inline bool        Contains(PM::Pointer address) const
    {
        return address >= m_Base
            && address < m_Base.Offset<PM::Pointer>(m_Size);
    }

  private:
    PM::Pointer m_Base = 0;
    usize       m_Size = 0;
};
