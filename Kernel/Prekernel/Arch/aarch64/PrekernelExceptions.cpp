/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Prekernel/Arch/aarch64/Prekernel.h>

#include <Kernel/Arch/aarch64/Aarch64Asm.h>

extern "C" void enter_el2_from_el3();
extern "C" void enter_el1_from_el2();

namespace Prekernel {

static void drop_to_el2()
{
    Kernel::Aarch64_SCR_EL3 secure_configuration_register_el3 = {};

    // secure_configuration_register_el3.ST = 1;  // Don't trap access to Counter-timer Physical Secure registers
    secure_configuration_register_el3.RW = 1;  // Lower level to use Aarch64
    secure_configuration_register_el3.NS = 1;  // Non-secure state
    secure_configuration_register_el3.HCE = 1; // Enable Hypervisor instructions at all levels

    Kernel::set_scr_el3(secure_configuration_register_el3);

    Kernel::Aarch64_SPSR_EL3 saved_program_status_register_el3 = {};

    // Mask (disable) all interrupts
    saved_program_status_register_el3.A = 1;
    saved_program_status_register_el3.I = 1;
    saved_program_status_register_el3.F = 1;
    saved_program_status_register_el3.D = 1;

    // Indicate EL1 as exception origin mode (so we go back there)
    saved_program_status_register_el3.M = Kernel::Aarch64_SPSR_EL3::Mode::EL2t;

    // Set the register
    Kernel::set_spsr_el3(saved_program_status_register_el3);

    // This will jump into os_start() below
    enter_el2_from_el3();
}
static void drop_to_el1()
{
    Kernel::Aarch64_HCR_EL2 hypervisor_configuration_register_el2 = {};
    hypervisor_configuration_register_el2.RW = 1; // EL1 to use 64-bit mode
    Kernel::set_hcr_el2(hypervisor_configuration_register_el2);

    Kernel::Aarch64_SPSR_EL2 saved_program_status_register_el2 = {};

    // Mask (disable) all interrupts
    saved_program_status_register_el2.A = 1;
    saved_program_status_register_el2.I = 1;
    saved_program_status_register_el2.F = 1;

    // Indicate EL1 as exception origin mode (so we go back there)
    saved_program_status_register_el2.M = Kernel::Aarch64_SPSR_EL2::Mode::EL1t;

    Kernel::set_spsr_el2(saved_program_status_register_el2);
    enter_el1_from_el2();
}

static void set_up_el1()
{
    Kernel::Aarch64_SCTLR_EL1 system_control_register_el1 = Kernel::Aarch64_SCTLR_EL1::Default();

    system_control_register_el1.UCT = 1;  // Don't trap access to CTR_EL0
    system_control_register_el1.nTWE = 1; // Don't trap WFE instructions
    system_control_register_el1.nTWI = 1; // Don't trap WFI instructions
    system_control_register_el1.DZE = 1;  // Don't trap DC ZVA instructions
    system_control_register_el1.UMA = 1;  // Don't trap access to DAIF (debugging) flags of EFLAGS register
    system_control_register_el1.SA0 = 1;  // Enable stack access alignment check for EL0
    system_control_register_el1.SA = 1;   // Enable stack access alignment check for EL1
    system_control_register_el1.A = 1;    // Enable memory access alignment check

    Kernel::set_sctlr_el1(system_control_register_el1);
}

void drop_to_exception_level_1()
{
    switch (Kernel::get_current_exception_level()) {
    case 3:
        drop_to_el2();
        [[fallthrough]];
    case 2:
        drop_to_el1();
        [[fallthrough]];
    case 1:
        set_up_el1();
        break;
    default: {
        Prekernel::panic("FATAL: CPU booted in unsupported exception mode!\r\n");
    }
    }
}

}
