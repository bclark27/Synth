#include "AtomicHelpers.h"

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


bool AtomicHelpers_TryGetLock(atomic_bool* lock)
{
    return !atomic_flag_test_and_set_explicit(lock, memory_order_acquire);
}

void AtomicHelpers_TryGetLockSpin(atomic_bool* lock)
{
    while (!AtomicHelpers_TryGetLock(lock)) {}
}

void AtomicHelpers_FreeLock(atomic_bool* lock)
{
    atomic_store(lock, 0);
}

