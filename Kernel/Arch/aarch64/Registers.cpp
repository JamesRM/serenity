/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/Registers.h>

namespace Kernel::Aarch64 {

TCR_EL1 TCR_EL1::Default()
{
    return {};
}
SCTLR_EL1 SCTLR_EL1::Default()
{
    SCTLR_EL1 system_control_register_el1 = {};
    system_control_register_el1.LSMAOE = 1;
    system_control_register_el1.nTLSMD = 1;
    system_control_register_el1.SPAN = 1;
    system_control_register_el1.IESB = 1;
    return system_control_register_el1;
}

}
