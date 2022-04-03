/*
 * Copyright (c) 2018-2022, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Memory.h>
#include <Kernel/Memory/MemoryManager.h>

extern multiboot_module_entry_t multiboot_copy_boot_modules_array[16];
extern size_t multiboot_copy_boot_modules_count;

namespace Kernel::Memory {

void build_physical_memory_map(Vector<UsedMemoryRange>& used_memory_ranges, Vector<PhysicalMemoryRange>& physical_memory_ranges, NonnullOwnPtrVector<PhysicalRegion>& user_physical_regions)
{
    if (multiboot_flags & 0x4) {
        auto* bootmods_start = multiboot_copy_boot_modules_array;
        auto* bootmods_end = bootmods_start + multiboot_copy_boot_modules_count;

        for (auto* bootmod = bootmods_start; bootmod < bootmods_end; bootmod++) {
            used_memory_ranges.append(UsedMemoryRange { UsedMemoryRangeType::BootModule, PhysicalAddress(bootmod->start), PhysicalAddress(bootmod->end) });
        }
    }

    auto* mmap_begin = multiboot_memory_map;
    auto* mmap_end = multiboot_memory_map + multiboot_memory_map_count;

    struct ContiguousPhysicalVirtualRange {
        PhysicalAddress lower;
        PhysicalAddress upper;
    };

    Vector<ContiguousPhysicalVirtualRange> contiguous_physical_ranges;

    for (auto* mmap = mmap_begin; mmap < mmap_end; mmap++) {
        // We have to copy these onto the stack, because we take a reference to these when printing them out,
        // and doing so on a packed struct field is UB.
        auto address = mmap->addr;
        auto length = mmap->len;
        ArmedScopeGuard write_back_guard = [&]() {
            mmap->addr = address;
            mmap->len = length;
        };

        dmesgln("MM: Multiboot mmap: address={:p}, length={}, type={}", address, length, mmap->type);

        auto start_address = PhysicalAddress(address);
        switch (mmap->type) {
        case (MULTIBOOT_MEMORY_AVAILABLE):
            physical_memory_ranges.append(PhysicalMemoryRange { PhysicalMemoryRangeType::Usable, start_address, length });
            break;
        case (MULTIBOOT_MEMORY_RESERVED):
            physical_memory_ranges.append(PhysicalMemoryRange { PhysicalMemoryRangeType::Reserved, start_address, length });
            break;
        case (MULTIBOOT_MEMORY_ACPI_RECLAIMABLE):
            physical_memory_ranges.append(PhysicalMemoryRange { PhysicalMemoryRangeType::ACPI_Reclaimable, start_address, length });
            break;
        case (MULTIBOOT_MEMORY_NVS):
            physical_memory_ranges.append(PhysicalMemoryRange { PhysicalMemoryRangeType::ACPI_NVS, start_address, length });
            break;
        case (MULTIBOOT_MEMORY_BADRAM):
            dmesgln("MM: Warning, detected bad memory range!");
            physical_memory_ranges.append(PhysicalMemoryRange { PhysicalMemoryRangeType::BadMemory, start_address, length });
            break;
        default:
            dbgln("MM: Unknown range!");
            physical_memory_ranges.append(PhysicalMemoryRange { PhysicalMemoryRangeType::Unknown, start_address, length });
            break;
        }

        if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        // Fix up unaligned memory regions.
        auto diff = (FlatPtr)address % PAGE_SIZE;
        if (diff != 0) {
            dmesgln("MM: Got an unaligned physical_region from the bootloader; correcting {:p} by {} bytes", address, diff);
            diff = PAGE_SIZE - diff;
            address += diff;
            length -= diff;
        }
        if ((length % PAGE_SIZE) != 0) {
            dmesgln("MM: Got an unaligned physical_region from the bootloader; correcting length {} by {} bytes", length, length % PAGE_SIZE);
            length -= length % PAGE_SIZE;
        }
        if (length < PAGE_SIZE) {
            dmesgln("MM: Memory physical_region from bootloader is too small; we want >= {} bytes, but got {} bytes", PAGE_SIZE, length);
            continue;
        }

        for (PhysicalSize page_base = address; page_base <= (address + length); page_base += PAGE_SIZE) {
            auto addr = PhysicalAddress(page_base);

            // Skip used memory ranges.
            bool should_skip = false;
            for (auto& used_range : used_memory_ranges) {
                if (addr.get() >= used_range.start.get() && addr.get() <= used_range.end.get()) {
                    should_skip = true;
                    break;
                }
            }
            if (should_skip)
                continue;

            if (contiguous_physical_ranges.is_empty() || contiguous_physical_ranges.last().upper.offset(PAGE_SIZE) != addr) {
                contiguous_physical_ranges.append(ContiguousPhysicalVirtualRange {
                    .lower = addr,
                    .upper = addr,
                });
            } else {
                contiguous_physical_ranges.last().upper = addr;
            }
        }
    }

    for (auto& range : contiguous_physical_ranges) {
        user_physical_regions.append(PhysicalRegion::try_create(range.lower, range.upper).release_nonnull());
    }
}

}
