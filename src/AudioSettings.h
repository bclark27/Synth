#ifndef AUDIO_SETTINGS_H_
#define AUDIO_SETTINGS_H_

///////////////
//  DEFINES  //
///////////////

/*
  STREAM_BUFFER_SIZE must divide SAMPLE_RATE
  and MODULE_BUFFER_SIZE must divide STREAM_BUFFER_SIZE
*/

#define SAMPLE_RATE                 44100
#define STREAM_BUFFER_SIZE          25
#define MODULE_BUFFER_SIZE          25
#define MODULE_BUFS_PER_STREAM_BUF  (STREAM_BUFFER_SIZE / MODULE_BUFFER_SIZE)
#define SEC_PER_SAMPLE              (1 / (double)SAMPLE_RATE)

#endif
