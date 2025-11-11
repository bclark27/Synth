#include "MIDI.h"

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

void MIDI_PopRingBuffer(MIDIData* ringBuffer, MIDIData* outBuffer, U4* write, U4* read)
{
    int i;
    for (i = 0; i < MIDI_STREAM_BUFFER_SIZE; i++)
    {
        U4 nextRead = (*read + 1) & (MIDI_STREAM_BUFFER_SIZE - 1);
        if (nextRead == *write) break;

        outBuffer[i] = ringBuffer[*read];
        *read = nextRead;
    }

    for (;i < MIDI_STREAM_BUFFER_SIZE; i++)
    {
        outBuffer[i].type = MIDIDataType_None;
    }
}

bool MIDI_PushRingBuffer(MIDIData* ringBuffer, MIDIData data, U4* write, U4* read)
{
    U4 nextWrite = (*write + 1) & (MIDI_STREAM_BUFFER_SIZE - 1);
    if (nextWrite == *read) return false;

    ringBuffer[nextWrite] = data;
    *write = nextWrite;
    return true;
}

MIDIData MIDI_PeakRingBuffer(MIDIData* ringBuffer, U4* read)
{
    return ringBuffer[*read];
}