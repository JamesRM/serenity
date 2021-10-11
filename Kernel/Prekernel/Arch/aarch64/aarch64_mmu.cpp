/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */


#include <Kernel/Prekernel/Arch/aarch64/aarch64_mmu.h>
#include <Kernel/Prekernel/Arch/aarch64/AarchRegisters.h>
#include <Kernel/Prekernel/Arch/aarch64/Aarch64Asm.h>


extern size_t __page_tables_size;

typedef unsigned char* t_page_table;
extern t_page_table __page_tables_phys_start;



void zero_identity_map(t_page_table page_table);
void build_identity_map(t_page_table page_table);
void activate_mmu();
void init_prekernel_page_table();
void switch_to_page_table(t_page_table page_table);


void zero_identity_map(t_page_table page_table) {
    // Memset all page table memory to zero
    for (unsigned char* p = page_table; 
         p < page_table + __page_tables_size; 
         p++) 
    {
        *p = 0;
    }
}

void build_identity_map(t_page_table page_table) {
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
        uint64_t* l3_entry = (uint64_t*)&page_table[TABLE_SIZE * 2 + l3_idx];
        
        *l3_entry = (uint64_t)&page_table[TABLE_SIZE * 2] + (l3_idx * TABLE_SIZE);
        *l3_entry |= MM_TABLE_DESCRIPTOR;
    }

    // Setup L4 entries
    for (size_t idx = 0, addr = 0; addr < 0x3F000000; addr += SECTION_SIZE, idx++) {
        uint64_t* l4_entry = (uint64_t*)&page_table[TABLE_SIZE * 3 + idx];
        
        *l4_entry = addr;
        *l4_entry |= MM_ACCESS;
        *l4_entry |= MM_TABLE_DESCRIPTOR;
        *l4_entry |= PT_ISH;
    }

    // Setup entries for last 16MB of memory (MMIO)
    for (size_t idx = 0, addr = 0x3F000000; addr < 0x3EFFFFFF; addr += SECTION_SIZE, idx++) {
        uint64_t* l4_entry = (uint64_t*)&page_table[TABLE_SIZE * 3 + idx];
        
        *l4_entry = addr;
        *l4_entry |= MM_ACCESS;
        *l4_entry |= MM_TABLE_DESCRIPTOR;
        *l4_entry |= PT_OSH;
        *l4_entry |= PT_DEV;
    }
}


void switch_to_page_table(t_page_table page_table) {
    set_ttbr0_el1((uint64_t)page_table);
    set_ttbr1_el1((uint64_t)page_table);
}

void activate_mmu() {
    Kernel::Aarch64_MAIR_EL1 mair_el1 = {};
    mair_el1.Attr[0] = 0xFF; //Normal memory
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


void init_prekernel_page_table() {
    zero_identity_map(__page_tables_phys_start);
    build_identity_map(__page_tables_phys_start);
    switch_to_page_table(__page_tables_phys_start);
    activate_mmu();
}