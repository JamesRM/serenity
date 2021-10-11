/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Arch/aarch64/Aarch64Registers.h>

namespace Kernel {

inline void set_ttbr1_el1(uint64_t ttbr1_el1)
{
    asm("msr ttbr1_el1, %[value]" ::[value] "r"(ttbr1_el1));
}

inline void set_ttbr0_el1(uint64_t ttbr0_el1)
{
    asm("msr ttbr0_el1, %[value]" ::[value] "r"(ttbr0_el1));
}

inline void set_mair_el1(Aarch64_MAIR_EL1 mair_el1)
{
    asm("msr mair_el1, %[value]" ::[value] "r"(mair_el1));
}

inline void set_sctlr_el1(Aarch64_SCTLR_EL1 sctlr_el1)
{
    asm("msr sctlr_el1, %[value]" ::[value] "r"(sctlr_el1));
}

inline Aarch64_SCTLR_EL1 get_sctlr_el1()
{
    Aarch64_SCTLR_EL1 sctlr;

    asm("mrs %[value], sctlr_el1"
        : [value] "=r"(sctlr));

    return sctlr;
}

inline void set_scr_el3(Aarch64_SCR_EL3 scr_el3)
{
    asm("msr scr_el3, %[value]" ::[value] "r"(scr_el3));
}

inline Aarch64_SCR_EL3 get_scr_el3()
{
    Aarch64_SCR_EL3 scr;

    asm("mrs %[value], scr_el3"
        : [value] "=r"(scr));

    return scr;
}

inline void set_spsr_el2(Aarch64_SPSR_EL2 spsr_el2)
{
    asm("msr spsr_el2, %[value]" ::[value] "r"(spsr_el2));
}

inline Aarch64_SPSR_EL2 get_spsr_el2()
{
    Aarch64_SPSR_EL2 spsr;

    asm("mrs %[value], spsr_el2"
        : [value] "=r"(spsr));

    return spsr;
}

inline void set_hcr_el2(Aarch64_HCR_EL2 hcr_el2)
{
    asm("msr hcr_el2, %[value]" ::[value] "r"(hcr_el2));
}

inline Aarch64_HCR_EL2 get_hcr_el2()
{
    Aarch64_HCR_EL2 spsr;

    asm("mrs %[value], hcr_el2"
        : [value] "=r"(spsr));

    return spsr;
}

inline void set_spsr_el3(Aarch64_SPSR_EL3 spsr_el3)
{
    asm("msr spsr_el3, %[value]" ::[value] "r"(spsr_el3));
}

inline Aarch64_SPSR_EL3 get_spsr_el3()
{
    Aarch64_SPSR_EL3 spsr;

    asm("mrs %[value], spsr_el3"
        : [value] "=r"(spsr));

    return spsr;
}

inline void set_tcr_el1(Aarch64_TCR_EL1 tcr_el1)
{
    asm("msr tcr_el1, %[value]" ::[value] "r"(tcr_el1));
}

inline Aarch64_ID_AA64MMFR0_EL1 get_id_aa64mmfr0_el1()
{
    Aarch64_ID_AA64MMFR0_EL1 feature_register;

    asm("mrs %[value], ID_AA64MMFR0_EL1"
        : [value] "=r"(feature_register));

    return feature_register;
}

inline void flush()
{

    asm("dsb ish");
    asm("isb");
}

[[noreturn]] inline void halt()
{
    for (;;) {
        asm volatile("wfi");
    }
}

typedef int Aarch64_ExceptionLevel;
Aarch64_ExceptionLevel get_current_exception_level();

}
