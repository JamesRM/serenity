/*
 * Copyright (c) 2021, Nico Weber <thakis@chromium.org>
 * Copyright (c) 2021, Marcin Undak <mcinek@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Types.h>

#include <Kernel/Arch/aarch64/Aarch64Asm.h>
#include <Kernel/Prekernel/Arch/aarch64/Aarch64_asm_utils.h>
#include <Kernel/Prekernel/Arch/aarch64/Mailbox.h>
#include <Kernel/Prekernel/Arch/aarch64/Prekernel.h>
#include <Kernel/Prekernel/Arch/aarch64/Timer.h>
#include <Kernel/Prekernel/Arch/aarch64/UART.h>

extern "C" [[noreturn]] void halt();
extern "C" void wait_cycles(int n);

extern "C" [[noreturn]] void init();
extern "C" [[noreturn]] void init()
{
    auto& uart = Prekernel::UART::the();

    uart.print_str("\r\nWelcome to Serenity OS!\r\n");
    uart.print_str("Imagine this being your ideal operating system.\r\n");
    uart.print_str("Observed deviations from that ideal are shortcomings of your imagination.\r\n\r\n");

    u32 firmware_version = Prekernel::Mailbox::query_firmware_version();
    uart.print_str("Firmware version: ");
    uart.print_num(firmware_version);
    uart.print_str("\r\n");

    uart.print_str("CPU started in: EL");
    uart.print_num(Kernel::get_current_exception_level());
    uart.print_str("\r\n");

    uart.print_str("Drop CPU to EL1\r\n");
    Prekernel::drop_to_exception_level_1();

    uart.print_str("Initialize MMU\r\n");
    Prekernel::init_prekernel_page_tables();

    uart.print_str("Enter loop\r\n");
    auto& timer = Prekernel::Timer::the();
    u64 start_musec = 0;
    for (;;) {
        u64 now_musec;
        while ((now_musec = timer.microseconds_since_boot()) - start_musec < 1'000'000)
            ;
        start_musec = now_musec;
        uart.print_str("Timer: ");
        uart.print_num(now_musec);
        uart.print_str("\r\n");
    }
}

// FIXME: Share this with the Intel Prekernel.
extern size_t __stack_chk_guard;
size_t __stack_chk_guard;
extern "C" [[noreturn]] void __stack_chk_fail();

[[noreturn]] void halt()
{
    for (;;) {
        asm volatile("wfi");
    }
}

void __stack_chk_fail()
{
    halt();
}

[[noreturn]] void __assertion_failed(char const*, char const*, unsigned int, char const*)
{
    halt();
}
