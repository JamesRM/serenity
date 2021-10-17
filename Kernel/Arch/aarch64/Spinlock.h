/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Atomic.h>
#include <AK/Noncopyable.h>
#include <AK/Types.h>
#include <Kernel/Arch/Processor.h>
#include <Kernel/Locking/LockRank.h>

namespace Kernel {

class Spinlock {
    AK_MAKE_NONCOPYABLE(Spinlock);
    AK_MAKE_NONMOVABLE(Spinlock);

public:
    // Spinlock(LockRank rank = LockRank::None)
    //     : m_rank(rank)
    // {
    // }
    Spinlock()
    {
    }
    Spinlock(LockRank)
    {
    }

    ALWAYS_INLINE u32 lock()
    {
        u32 prev_flags = 0;          //cpu_flags();
        Processor::enter_critical(); //Disabled interrupts?
        //cli();
        while (m_lock.exchange(1, AK::memory_order_acquire) != 0) {
            Processor::wait_check();
        }
        //track_lock_acquire(m_rank);
        return prev_flags;
        return 0;
    }

    ALWAYS_INLINE void unlock(u32 /*prev_flags*/)
    {
        //VERIFY(is_locked());
        //track_lock_release(m_rank);
        m_lock.store(0, AK::memory_order_release);
        // if (prev_flags & 0x200)
        //     sti();
        // else
        //     cli();

        Processor::leave_critical(); //Re-enable interrupts?
    }

    [[nodiscard]] ALWAYS_INLINE bool is_locked() const
    {
        return m_lock.load(AK::memory_order_relaxed) != 0;
    }

    ALWAYS_INLINE void initialize()
    {
        m_lock.store(0, AK::memory_order_relaxed);
    }

private:
    Atomic<u8> m_lock { 0 };
    // const LockRank m_rank;
};

class RecursiveSpinlock {
    AK_MAKE_NONCOPYABLE(RecursiveSpinlock);
    AK_MAKE_NONMOVABLE(RecursiveSpinlock);

public:
    RecursiveSpinlock(LockRank rank = LockRank::None)
    {
        (void)rank;
    }

    ALWAYS_INLINE u32 lock()
    {
        return 0;
    }

    ALWAYS_INLINE void unlock(u32 /*prev_flags*/)
    {
    }

    [[nodiscard]] ALWAYS_INLINE bool is_locked() const
    {
        return false;
    }

    [[nodiscard]] ALWAYS_INLINE bool is_locked_by_current_processor() const
    {
        return false;
    }

    ALWAYS_INLINE void initialize()
    {
    }
};

}
