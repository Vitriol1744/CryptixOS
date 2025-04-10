/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/StorageDevice.hpp>

void StorageDevice::LoadPartitionTable() { m_PartitionTable.Load(*this); };
