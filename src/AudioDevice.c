#include "soundio/soundio.h"
#include "AudioSettings.h"
#include "ModularSynth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


static float synth_buffer[STREAM_BUFFER_SIZE * 2]; // stereo interleaved
static int synth_pos = 0;        // current playback position in frames

// ---- Your synth update ----
// This fills synth_buffer with SYNTH_BUFFER_FRAMES of stereo samples.
static void update_synth() {
    
    R4* left = ModularSynth_getLeftChannel();
    ModularSynth_update();

    for (int i = 0; i < STREAM_BUFFER_SIZE; i++)
    {
      synth_buffer[i * 2] = left[i];
      synth_buffer[i * 2 + 1] = left[i];
    }

    synth_pos = 0;
}

// ---- SoundIO callback ----
static void write_callback(struct SoundIoOutStream *outstream,
                           int frame_count_min, int frame_count_max)
{
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    struct SoundIoChannelArea *areas;
    int frames_left = frame_count_max;
    int err;

    while (frames_left > 0) {
        int frame_count = frames_left;
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "begin_write: %s\n", soundio_strerror(err));
            exit(1);
        }
        if (!frame_count)
            break;

        float *left_ptr, *right_ptr;
        for (int frame = 0; frame < frame_count; frame++) {
            // if synth buffer empty, refill it
            if (synth_pos >= STREAM_BUFFER_SIZE)
            {
              update_synth();
              synth_pos = 0;
            }

            float left = synth_buffer[synth_pos * 2 + 0];
            float right = synth_buffer[synth_pos * 2 + 1];
            synth_pos++;

            // write to all channels
            for (int ch = 0; ch < layout->channel_count; ch++) {
                float *ptr = (float *)(areas[ch].ptr + areas[ch].step * frame);
                *ptr = (ch == 0) ? left : right;
            }
        }

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "end_write: %s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
    }
}

int main() {

    ModularSynth_init();
    ModularSynth_readConfig("/home/ben/projects/github/my/Synth/config/synth2");

    int err;
    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if ((err = soundio_connect(soundio))) {
        fprintf(stderr, "error connecting: %s", soundio_strerror(err));
        return 1;
    }

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    if (default_out_device_index < 0) {
        fprintf(stderr, "no output device found");
        return 1;
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, default_out_device_index);
    if (!device) {
        fprintf(stderr, "out of memory");
        return 1;
    }

    fprintf(stderr, "Output device: %s\n", device->name);

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    outstream->format = SoundIoFormatFloat32NE;
    outstream->write_callback = write_callback;
    outstream->sample_rate = SAMPLE_RATE;
    outstream->software_latency = (double)STREAM_BUFFER_SIZE / SAMPLE_RATE;

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return 1;
    }

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start device: %s", soundio_strerror(err));
        return 1;
    }

    for (;;)
        soundio_wait_events(soundio);

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}