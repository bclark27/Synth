#ifndef BYTE_ARRAY_HELPERS_H_
#define BYTE_ARRAY_HELPERS_H_

#include "comm/Common.h"
#include <stdatomic.h>

///////////////
//  DEFINES  //
///////////////

///////////
// TYPES //
///////////

typedef struct ByteArray
{
  unsigned int length;
  char* bytes;
} ByteArray;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

bool ByteArrayHelpers_TryGetLock(atomic_bool* lock);
void ByteArrayHelpers_FreeLock(atomic_bool* lock);

#endif