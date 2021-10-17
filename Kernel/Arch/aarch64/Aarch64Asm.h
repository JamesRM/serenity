/*
 * Copyright (c) 2021, James Mintram <me@jamesrm.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Arch/aarch64/Aarch64Registers.h>

namespace Kernel {

[[noreturn]] inline void halt()
{
    for (;;) {
        asm volatile("wfi");
    }
}

typedef int Aarch64_ExceptionLevel;
Aarch64_ExceptionLevel get_current_exception_level();

}
