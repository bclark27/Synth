#include "MIDI.h"

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

static inline bool __MIDI_Pop(MIDIData* ringBuffer, MIDIData *out, atomic_uint* write_idx, atomic_uint* read_idx) {
    unsigned int read  = atomic_load_explicit(read_idx, memory_order_relaxed);
    unsigned int write = atomic_load_explicit(write_idx, memory_order_acquire);

    if (read == write) {
        // Buffer empty
        return false;
    }

    *out = ringBuffer[read];
    unsigned int next = (read + 1) & (MIDI_STREAM_BUFFER_SIZE - 1);

    atomic_store_explicit(read_idx, next, memory_order_release);
    return true;
}




void MIDI_PopRingBuffer(MIDIData* ringBuffer, MIDIData* outBuffer, atomic_uint* write_idx, atomic_uint* read_idx)
{
    int i;
    for (i = 0; i < MIDI_STREAM_BUFFER_SIZE; i++)
    {
        bool popped = __MIDI_Pop(ringBuffer, outBuffer + i, write_idx, read_idx);
        if (!popped) break;
    }

    for (; i < MIDI_STREAM_BUFFER_SIZE; i++)
        outBuffer[i].type = MIDIDataType_None;
}

bool MIDI_PushRingBuffer(MIDIData* ringBuffer, MIDIData data, atomic_uint* write_idx, atomic_uint* read_idx)
{
    unsigned int write = atomic_load_explicit(write_idx, memory_order_relaxed);
    unsigned int read  = atomic_load_explicit(read_idx, memory_order_acquire);
    unsigned int next  = (write + 1) & (MIDI_STREAM_BUFFER_SIZE - 1);

    if (next == read) {
        // Buffer full
        return false;
    }

    ringBuffer[write] = data;

    // Publish the new write index
    atomic_store_explicit(write_idx, next, memory_order_release);
    return true;
}

MIDIData MIDI_PeakRingBuffer(MIDIData* ringBuffer, atomic_uint* read)
{
    unsigned int r = atomic_load_explicit(read, memory_order_acquire);
    return ringBuffer[r];
}