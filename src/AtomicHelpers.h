
#ifndef ATOMIC_HELPERS_H_
#define ATOMIC_HELPERS_H_

#include "comm/Common.h"
#include <stdatomic.h>

///////////////
//  DEFINES  //
///////////////

///////////
// TYPES //
///////////

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

bool AtomicHelpers_TryGetLock(atomic_bool* lock);
void AtomicHelpers_TryGetLockSpin(atomic_bool* lock);
void AtomicHelpers_FreeLock(atomic_bool* lock);

#endif