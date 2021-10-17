/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/Aarch64Asm.h>
#include <Kernel/Prekernel/Arch/aarch64/UART.h>

namespace Kernel {

Aarch64_ExceptionLevel get_current_exception_level()
{
    Aarch64_ExceptionLevel current_exception_level;

    asm("mrs  %[value], CurrentEL"
        : [value] "=r"(current_exception_level));

    current_exception_level = (current_exception_level >> 2) & 0x3;
    return current_exception_level;
}

}
