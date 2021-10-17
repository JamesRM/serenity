/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Prekernel/Arch/aarch64/AarchRegisters.h>

inline void set_ttbr1_el1(uint64_t ttbr1_el1)
{
    asm("msr ttbr1_el1, %[value]" ::[value] "r"(ttbr1_el1));
}

inline void set_ttbr0_el1(uint64_t ttbr0_el1)
{
    asm("msr ttbr0_el1, %[value]" ::[value] "r"(ttbr0_el1));
}

inline void set_mair_el1(Kernel::Aarch64_MAIR_EL1 mair_el1)
{
    asm("msr mair_el1, %[value]" ::[value] "r"(mair_el1));
}

inline void set_sctlr_el1(Kernel::Aarch64_SCTLR_EL1 sctlr_el1)
{
    asm("msr sctlr_el1, %[value]" ::[value] "r"(sctlr_el1));
}

inline Kernel::Aarch64_SCTLR_EL1 get_sctlr_el1()
{
    Kernel::Aarch64_SCTLR_EL1 sctlr;

    asm("mrs %[value], sctlr_el1"
        : [value] "=r"(sctlr));

    return sctlr;
}

inline void set_tcr_el1(Kernel::Aarch64_TCR_EL1 tcr_el1)
{
    asm("msr tcr_el1, %[value]" ::[value] "r"(tcr_el1));
}

// uint64_t get_id_aa64mmfr0_el1() {
//     asm("msr id_aa64mmfr0_el1, %[value]" ::[value] "r"(id_aa64mmfr0_el1));
// }

inline void flush()
{

    asm("dsb ish");
    asm("isb");
}
