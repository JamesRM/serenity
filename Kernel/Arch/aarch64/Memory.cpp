/*
 * Copyright (c) 2018-2022, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Memory.h>

namespace Kernel::Memory {

void build_physical_memory_map(Vector<UsedMemoryRange>&, Vector<PhysicalMemoryRange>&, NonnullOwnPtrVector<PhysicalRegion>&)
{
}

}
