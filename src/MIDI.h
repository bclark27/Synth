#ifndef MIDI_H_
#define MIDI_H_


#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

#define MAX_NOTES   5

///////////
// TYPES //
///////////

typedef struct MIDIData
{
    R4 note[MAX_NOTES];
    bool noteIsOn[MAX_NOTES];
    bool noteIsNewlyOn[MAX_NOTES];
    R4 velocity[MAX_NOTES];
} MIDIData;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////


#endif