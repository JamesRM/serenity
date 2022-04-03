/*
 * Copyright (c) 2018-2022, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include<AK/Vector.h>

namespace Kernel::Memory {

struct UsedMemoryRange;
struct PhysicalMemoryRange;
struct PhysicalRegion;

void build_physical_memory_map(Vector<UsedMemoryRange> &, Vector<PhysicalMemoryRange> &, NonnullOwnPtrVector<PhysicalRegion> &);

}
