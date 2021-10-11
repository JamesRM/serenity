/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Types.h>

#include <Kernel/Prekernel/Arch/aarch64/Prekernel.h>

#include <Kernel/Arch/aarch64/Aarch64Asm.h>
#include <Kernel/Arch/aarch64/Aarch64Registers.h>
#include <Kernel/Prekernel/Arch/aarch64/UART.h>

#define TABLE_SHIFT 9                     //9 bits of address space per table (512 entries)
#define PAGE_SHIFT 12                     //4096 bytes per page - lower 12 bits
#define SECTION_SHIFT PAGE_SHIFT          //Bits remaining for the offset within a 2MB section (21 for 2MB, 12 for 4k)
#define SECTION_SIZE (1 << SECTION_SHIFT) //21 Bits of address = 2MB, 9 Bits of address = 4k

#define TABLE_SIZE 0x1000

#define MM_BLOCK_DESCRIPTOR 0b01
#define MM_TABLE_DESCRIPTOR 0b11

#define MM_ACCESS (0x1 << 10)

// physcial memory
constexpr u32 START_OF_NORMAL_MEMORY = 0x00000000;
constexpr u32 END_OF_NORMAL_MEMORY = 0x3EFFFFFF;
constexpr u32 START_OF_DEVICE_MEMORY = 0x3F000000;
constexpr u32 END_OF_DEVICE_MEMORY = 0x3FFFFFFF;

// shareability
constexpr u32 OUTER_SHAREABLE = (2 << 8);
constexpr u32 INNER_SHAREABLE = (3 << 8);

constexpr u32 NORMAL_MEMORY = (0 << 2);
constexpr u32 DEVICE_MEMORY = (1 << 2);

// These come from the linker script
extern size_t page_tables_size;
extern unsigned char page_tables_phys_start[];

namespace Prekernel {

typedef unsigned char* t_page_table;

static void zero_identity_map(t_page_table page_table)
{
    // Memset all page table memory to zero
    for (uint64_t* p = (uint64_t*)page_table;
         p < (uint64_t*)(page_table + page_tables_size);
         p++) {

        *p = 0;
    }
}

static void build_identity_map(t_page_table page_table)
{
    // Setup first entry of PGD
    uint64_t* pgd_entry = (uint64_t*)page_table;
    *pgd_entry = (uint64_t)&page_table[TABLE_SIZE];
    *pgd_entry |= MM_TABLE_DESCRIPTOR;

    // Setup first entry of PUD
    uint64_t* pug_entry = (uint64_t*)&page_table[TABLE_SIZE];
    *pug_entry = (uint64_t)&page_table[TABLE_SIZE * 2];
    *pug_entry |= MM_TABLE_DESCRIPTOR;

    // Setup L3 entries
    for (uint32_t l3_idx = 0; l3_idx < 512; l3_idx++) {
        uint64_t* l3_entry = (uint64_t*)&page_table[TABLE_SIZE * 2 + (l3_idx * sizeof(uint64_t))];

        *l3_entry = (uint64_t)(page_table + (TABLE_SIZE * 3) + (l3_idx * TABLE_SIZE));
        *l3_entry |= MM_TABLE_DESCRIPTOR;
    }

    // Setup L4 entries
    size_t page_index = 0;

    for (size_t addr = START_OF_NORMAL_MEMORY; addr < END_OF_NORMAL_MEMORY; addr += SECTION_SIZE, page_index++) {
        uint64_t* l4_entry = (uint64_t*)&page_table[TABLE_SIZE * 3 + (page_index * sizeof(uint64_t))];

        *l4_entry = addr;
        *l4_entry |= MM_ACCESS;
        *l4_entry |= MM_TABLE_DESCRIPTOR;
        *l4_entry |= INNER_SHAREABLE;
        *l4_entry |= NORMAL_MEMORY;
    }

    // Setup entries for last 16MB of memory (MMIO)
    for (size_t addr = START_OF_DEVICE_MEMORY; addr < END_OF_DEVICE_MEMORY; addr += SECTION_SIZE, page_index++) {
        uint64_t* l4_entry = (uint64_t*)&page_table[TABLE_SIZE * 3 + (page_index * sizeof(uint64_t))];

        *l4_entry = addr;
        *l4_entry |= MM_ACCESS;
        *l4_entry |= MM_TABLE_DESCRIPTOR;
        *l4_entry |= OUTER_SHAREABLE;
        *l4_entry |= DEVICE_MEMORY;
    }
}

static void switch_to_page_table(t_page_table page_table)
{
    Kernel::set_ttbr0_el1((uint64_t)page_table);
    Kernel::set_ttbr1_el1((uint64_t)page_table);
}

static void activate_mmu()
{
    Kernel::Aarch64_MAIR_EL1 mair_el1 = {};
    mair_el1.Attr[0] = 0xFF;       //Normal memory
    mair_el1.Attr[1] = 0b00000100; // Device-nGnRE memory (non-cacheble)
    set_mair_el1(mair_el1);

    // Configure cacheability attributes for memory associated with translation table walks
    Kernel::Aarch64_TCR_EL1 tcr_el1 = {};

    tcr_el1.SH1 = Kernel::Aarch64_TCR_EL1::InnerShareable;
    tcr_el1.ORGN1 = Kernel::Aarch64_TCR_EL1::NormalMemory_Outer_WriteBack_ReadAllocate_WriteAllocateCacheable;
    tcr_el1.IRGN1 = Kernel::Aarch64_TCR_EL1::NormalMemory_Inner_WriteBack_ReadAllocate_WriteAllocateCacheable;

    tcr_el1.SH0 = Kernel::Aarch64_TCR_EL1::InnerShareable;
    tcr_el1.ORGN0 = Kernel::Aarch64_TCR_EL1::NormalMemory_Outer_WriteBack_ReadAllocate_WriteAllocateCacheable;
    tcr_el1.IRGN0 = Kernel::Aarch64_TCR_EL1::NormalMemory_Inner_WriteBack_ReadAllocate_WriteAllocateCacheable;

    tcr_el1.TG1 = Kernel::Aarch64_TCR_EL1::TG1_Size_4KB;
    tcr_el1.TG0 = Kernel::Aarch64_TCR_EL1::TG0_Size_4KB;

    // Auto detect the Intermediate Physical Address Size
    Kernel::Aarch64_ID_AA64MMFR0_EL1 feature_register = Kernel::get_id_aa64mmfr0_el1();
    tcr_el1.IPS = feature_register.PARange;

    set_tcr_el1(tcr_el1);

    // Enable MMU in the system control register
    Kernel::Aarch64_SCTLR_EL1 sctlr_el1 = Kernel::get_sctlr_el1();
    sctlr_el1.M = 1; //Enable MMU
    Kernel::set_sctlr_el1(sctlr_el1);

    Kernel::flush();
}

void init_prekernel_page_tables()
{
    zero_identity_map(page_tables_phys_start);
    build_identity_map(page_tables_phys_start);
    switch_to_page_table(page_tables_phys_start);
    activate_mmu();
}

}
