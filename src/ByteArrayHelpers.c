#include "ByteArrayHelpers.h"

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


bool ByteArrayHelpers_TryGetLock(atomic_bool* lock)
{
    return !atomic_flag_test_and_set_explicit(lock, memory_order_acquire);
}

void ByteArrayHelpers_FreeLock(atomic_bool* lock)
{
    atomic_store(lock, 0);
}