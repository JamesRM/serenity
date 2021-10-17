/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/Aarch64Registers.h>

namespace Kernel {

Aarch64_TCR_EL1 Aarch64_TCR_EL1::Default()
{
    return {};
}
Aarch64_SCTLR_EL1 Aarch64_SCTLR_EL1::Default()
{
    Aarch64_SCTLR_EL1 system_control_register_el1 = {};
    system_control_register_el1.LSMAOE = 1;
    system_control_register_el1.nTLSMD = 1;
    system_control_register_el1.SPAN = 1;
    system_control_register_el1.IESB = 1;
    return system_control_register_el1;
}

}
