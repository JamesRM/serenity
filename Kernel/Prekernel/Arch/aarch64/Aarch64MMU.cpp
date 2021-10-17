/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Prekernel/Arch/aarch64/Aarch64Asm.h>
#include <Kernel/Prekernel/Arch/aarch64/AarchRegisters.h>
#include <Kernel/Prekernel/Arch/aarch64/UART.h>

#define TABLE_SHIFT 9                     //9 bits of address space per table (512 entries)
#define PAGE_SHIFT 12                     //4096 bytes per page - lower 12 bits
#define SECTION_SHIFT PAGE_SHIFT          //Bits remaining for the offset within a 2MB section (21 for 2MB, 12 for 4k)
#define SECTION_SIZE (1 << SECTION_SHIFT) //21 Bits of address = 2MB, 9 Bits of address = 4k

#define TABLE_SIZE 0x1000

#define MM_TABLE_DESCRIPTOR 0b11

#define MM_ACCESS (0x1 << 10)

// shareability
#define PT_OSH (2 << 8) // outter shareable
#define PT_ISH (3 << 8) // inner shareable

#define PT_MEM (0 << 2) // normal memory
#define PT_DEV (1 << 2) // device memory

extern size_t page_tables_size;

typedef unsigned char* t_page_table;
extern unsigned char page_tables_phys_start;

void zero_identity_map(t_page_table page_table);
void build_identity_map(t_page_table page_table);
void activate_mmu();
void init_prekernel_page_table();
void switch_to_page_table(t_page_table page_table);

__attribute__((no_sanitize("undefined"))) void zero_identity_map(t_page_table page_table)
{
    // Memset all page table memory to zero
    for (uint64_t* p = (uint64_t*)page_table;
         p < (uint64_t*)(page_table + page_tables_size);
         p++) {

        *p = 0;
    }
}

//__attribute__((no_sanitize("undefined")))
void build_identity_map(t_page_table page_table)
{
    auto& uart = Prekernel::UART::the();

    uart.print_str("PGD\r\n");

    // Setup first entry of PGD
    uint64_t* pgd_entry = (uint64_t*)page_table;
    *pgd_entry = (uint64_t)&page_table[TABLE_SIZE];
    *pgd_entry |= MM_TABLE_DESCRIPTOR;

    uart.print_str("PUG\r\n");
    // Setup first entry of PUD
    uint64_t* pug_entry = (uint64_t*)&page_table[TABLE_SIZE];
    *pug_entry = (uint64_t)&page_table[TABLE_SIZE * 2];
    *pug_entry |= MM_TABLE_DESCRIPTOR;

    uart.print_str("L3\r\n");
    // Setup L3 entries
    for (uint32_t l3_idx = 0; l3_idx < 512; l3_idx++) {
        uint64_t* l3_entry = (uint64_t*)&page_table[TABLE_SIZE * 2 + (l3_idx * sizeof(uint64_t))];

        *l3_entry = (uint64_t)(page_table + (TABLE_SIZE * 2) + (l3_idx * TABLE_SIZE));
        *l3_entry |= MM_TABLE_DESCRIPTOR;
    }

    uart.print_str("PGL4\r\n");
    // Setup L4 entries
    for (size_t idx = 0, addr = 0; addr < 0x3F000000; addr += SECTION_SIZE, idx++) {
        uint64_t* l4_entry = (uint64_t*)&page_table[TABLE_SIZE * 3 + (idx * sizeof(uint64_t))];

        *l4_entry = addr;
        *l4_entry |= MM_ACCESS;
        *l4_entry |= MM_TABLE_DESCRIPTOR;
        *l4_entry |= PT_ISH;
    }

    uart.print_str("Device\r\n");
    // Setup entries for last 16MB of memory (MMIO)
    for (size_t idx = 0, addr = 0x3F000000; addr < 0x3EFFFFFF; addr += SECTION_SIZE, idx++) {
        uint64_t* l4_entry = (uint64_t*)&page_table[TABLE_SIZE * 3 + (idx * sizeof(uint64_t))];

        *l4_entry = addr;
        *l4_entry |= MM_ACCESS;
        *l4_entry |= MM_TABLE_DESCRIPTOR;
        *l4_entry |= PT_OSH;
        *l4_entry |= PT_DEV;
    }
}

void switch_to_page_table(t_page_table page_table)
{
    set_ttbr0_el1((uint64_t)page_table);
    set_ttbr1_el1((uint64_t)page_table);
}

void activate_mmu()
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

    // TODO/FIXME Setup TCR according to PARange bits from ID_AA64MMFR0_EL1.

    set_tcr_el1(tcr_el1);

    // Enable MMU in the system control register
    Kernel::Aarch64_SCTLR_EL1 sctlr_el1 = get_sctlr_el1();
    sctlr_el1.M = 1;
    set_sctlr_el1(sctlr_el1);

    flush();
}

void init_prekernel_page_tables();
void init_prekernel_page_tables()
{
    auto& uart = Prekernel::UART::the();

    uart.print_str("zero_identity_map\r\n");
    zero_identity_map(&page_tables_phys_start);

    // uart.print_str("build_identity_map\r\n");
    // build_identity_map(&page_tables_phys_start);

    uart.print_str("switch_to_page_table\r\n");
    switch_to_page_table(&page_tables_phys_start);

    uart.print_str("activate_mmu\r\n");
    activate_mmu();
}
