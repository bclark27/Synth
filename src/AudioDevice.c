#include "AudioDevice.h"
#include "AudioSettings.h"
#include "ModularSynth.h"
#include "comm/Common.h"
#include <time.h>


///////////////
//  DEFINES  //
///////////////

#define SHOW_TIME

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void update_synth();
static void write_callback(struct SoundIoOutStream *outstream,
    int frame_count_min, int frame_count_max);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static float synth_buffer[STREAM_BUFFER_SIZE * 2]; // stereo interleaved
static int synth_pos = 0;        // current playback position in frames

static AudioDevice auddev_inst;
static AudioDevice* auddev = &auddev_inst;


//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////





void AudioDevice_init(void) {

    memset(auddev, 0, sizeof(AudioDevice));

    int err;
    auddev->soundio = soundio_create();
    if (!auddev->soundio) {
        fprintf(stderr, "out of memory\n");
        return;
    }

    if ((err = soundio_connect(auddev->soundio))) {
        fprintf(stderr, "error connecting: %s", soundio_strerror(err));
        return;
    }

    soundio_flush_events(auddev->soundio);

    int default_out_device_index = soundio_default_output_device_index(auddev->soundio);
    if (default_out_device_index < 0) {
        fprintf(stderr, "no output device found");
        return;
    }

    auddev->device = soundio_get_output_device(auddev->soundio, default_out_device_index);
    if (!auddev->device) {
        fprintf(stderr, "out of memory");
        return;
    }

    fprintf(stderr, "Output device: %s\n", auddev->device->name);

    auddev->outstream = soundio_outstream_create(auddev->device);
    auddev->outstream->format = SoundIoFormatFloat32NE;
    auddev->outstream->write_callback = write_callback;
    auddev->outstream->sample_rate = SAMPLE_RATE;
    auddev->outstream->software_latency = (double)STREAM_BUFFER_SIZE / SAMPLE_RATE;

    if ((err = soundio_outstream_open(auddev->outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return;
    }

    if (auddev->outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(auddev->outstream->layout_error));

    if ((err = soundio_outstream_start(auddev->outstream))) {
        fprintf(stderr, "unable to start device: %s", soundio_strerror(err));
        return;
    }
}


void AudioDevice_LoopForever(void)
{


    for (;;)
        soundio_wait_events(auddev->soundio);

    soundio_outstream_destroy(auddev->outstream);
    soundio_device_unref(auddev->device);
    soundio_destroy(auddev->soundio);
    return;
}


/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

struct timespec start, end;
static double total_time = 0.0;
static int count = 0;
static double min_time = 9999999999;
static double max_time = 0;

// This fills synth_buffer with SYNTH_BUFFER_FRAMES of stereo samples.
static void update_synth() {
    
    R4* left = ModularSynth_getLeftChannel();

#ifdef SHOW_TIME
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif

    ModularSynth_update();

#ifdef SHOW_TIME
    clock_gettime(CLOCK_MONOTONIC, &end);
    double delta = (end.tv_sec - start.tv_sec) * 1e6 +
                   (end.tv_nsec - start.tv_nsec) / 1e3;

    min_time = MIN(min_time, delta);
    max_time = MAX(max_time, delta);
    total_time += delta;
    count++;

    static double last_print = 0.0;
    double now = end.tv_sec + end.tv_nsec / 1e9;
    if (now - last_print >= 1.0) {
        double avg = (total_time / count) / 1000.f;
        double mn = min_time / 1000.f;
        double mx = max_time / 1000.f;
        printf("===\nAvg synth_update time: %.2f ms (%d calls)\n", avg, count);
        double buffer_time_ms = ((double)STREAM_BUFFER_SIZE / SAMPLE_RATE) * 1000;
        printf("time to beat: %lf ms\n", buffer_time_ms);
        printf("avg was %.3lfx faster\n", buffer_time_ms / avg);
        printf("min was %.3lfx faster\n", buffer_time_ms / mn);
        printf("max was %.3lfx faster\n", buffer_time_ms / mx);
        total_time = 0.0;
        count = 0;
        min_time = 9999999999;
        max_time = 0;
        last_print = now;
    }
#endif

    for (int i = 0; i < STREAM_BUFFER_SIZE; i++)
    {
      synth_buffer[i * 2] = left[i];
      synth_buffer[i * 2 + 1] = left[i];
    }

    synth_pos = 0;
}

// TODO: if ever got crackle or pop sounds, just start double buffering this guy here

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
