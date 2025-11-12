#ifndef MIDI_H_
#define MIDI_H_


#include "comm/Common.h"

///////////////
//  DEFINES  //
///////////////

#define MIDI_STREAM_BUFFER_SIZE   32 // pow of 2

///////////
// TYPES //
///////////

/**
 * @enum MIDIDataType
 * @brief Core MIDI message types handled by the synth.
 *
 * Each MIDI message begins with a status byte that encodes both the
 * message type (upper nibble) and the channel number (lower nibble).
 *
 * The data bytes that follow depend on the message type:
 *
 *   [Status] [Data1] [Data2]
 *
 * Example:
 *   0x90 0x3C 0x64  →  Note On, channel 0, note 60 (C4), velocity 100
 */
typedef enum MIDIDataType
{
    /**
     * @brief Note Off
     *
     * Structure:
     *   Status: 0x0
     *
     * Indicates that there is no data here anymore. used module inter connections and midi input module control reset
     */
    MIDIDataType_None = 0x0,

    /**
     * @brief Note Off
     *
     * Structure:
     *   Status: 0x80–0x8F
     *   Data1: Note number (0–127)
     *   Data2: Release velocity (0–127)
     *
     * Indicates that a note has been released on the specified channel.
     */
    MIDIDataType_NoteOff = 0x80,

    /**
     * @brief Note On
     *
     * Structure:
     *   Status: 0x90–0x9F
     *   Data1: Note number (0–127)
     *   Data2: Velocity (0–127)
     *
     * Starts a note. If velocity == 0, it should be treated as a Note Off.
     */
    MIDIDataType_NoteOn = 0x90,

    /**
     * @brief Polyphonic Aftertouch (Poly Pressure)
     *
     * Structure:
     *   Status: 0xA0–0xAF
     *   Data1: Note number (0–127)
     *   Data2: Pressure amount (0–127)
     *
     * Sent when a held key's pressure changes. Applies to a single note.
     */
    MIDIDataType_AfterTouch = 0xA0,

    /**
     * @brief Pitch Bend Change
     *
     * Structure:
     *   Status: 0xE0–0xEF
     *   Pitchbend: (0–16383)
     * 
     * Center position is 8192 (no bend).
     * Range is 0 (full down) to 16383 (full up).
     */
    MIDIDataType_PitchBend = 0xE0,

} MIDIDataType;


/**
 * @struct MIDIData
 * @brief Decoded representation of a single MIDI message.
 *
 * This struct holds the decoded form of a MIDI message after parsing.
 * Only some fields are meaningful depending on `type`.
 *
 * For example:
 *   - Note On/Off: use `data1` = note, `data2` = velocity.
 *   - Aftertouch:  use `data1` = note, `data2` = pressure.
 *   - Pitch Bend:  use `pitchbend` (0–16383).
 */
typedef struct MIDIData
{
    MIDIDataType type;     /**< Message type (Note On, Note Off, etc.) */
    unsigned short pitchbend; /**< 14-bit pitch bend value (0–16383). Used only for PitchBend. */
    unsigned char channel; /**< Channel number (0–15). Lower nibble of status byte. */
    unsigned char data1;   /**< First data byte. Meaning depends on message type. */
    unsigned char data2;   /**< Second data byte. Meaning depends on message type. */
} MIDIData;

typedef struct MIDIDataRingBuffer
{
    MIDIData buffer[MIDI_STREAM_BUFFER_SIZE];
    U2 read;
    U2 write;
} MIDIDataRingBuffer;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void MIDI_PopRingBuffer(MIDIData* ringBuffer, MIDIData* outBuffer, U4* write, U4* read);
bool MIDI_PushRingBuffer(MIDIData* ringBuffer, MIDIData data, U4* write, U4* read);
MIDIData MIDI_PeakRingBuffer(MIDIData* ringBuffer, U4* read);

#endif