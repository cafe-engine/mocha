#ifndef MOCHA_H
#define MOCHA_H

// #include "definitions.h"

#include <stdio.h>
#include <stdlib.h>

#define TRACEERR(...) int _;(void)_
#define ERRLOG(...) int _;(void)_

#define TRACELOG(...) int _;(void)_
#define LOG(...) int _;(void)_

#define MO_API extern
#define MOCHA_VALUE float

#define MAX_AUDIO_DATA 64

// #include "miniaudio.h"

#ifndef AUDIO_DEVICE_FORMAT
  #define AUDIO_DEVICE_FORMAT ma_format_f32
#endif

#ifndef AUDIO_DEVICE_CHANNELS
  #define AUDIO_DEVICE_CHANNELS 2
#endif

#ifndef AUDIO_DEVICE_SAMPLE_RATE
  #define AUDIO_DEVICE_SAMPLE_RATE 44100
#endif

#ifndef MAX_AUDIO_BUFFER_CHANNELS
  #define MAX_AUDIO_BUFFER_CHANNELS 255
#endif


typedef enum {
  MO_AUDIO_STREAM = 0,
  MO_AUDIO_STATIC
} MO_AUDIO_USAGE_;


typedef struct Mocha Mocha;
typedef struct mo_AudioData mo_AudioData;
typedef struct mo_AudioBuffer mo_AudioBuffer;
// typedef unsigned int mo_Sound;

struct mo_AudioData {
  MO_AUDIO_USAGE_ usage;
  unsigned char *data;
  unsigned char *filename;
  size_t size;
  int refs;
};



typedef struct mo_Sound {
  mo_AudioBuffer *audioBuffer;
} mo_Sound;

// typedef struct {
//   struct
//   {
//     ma_context ctx;
//     ma_device device;
//     ma_mutex lock;

//     int isReady;

//   } system;

//   struct
//   {
//     mo_AudioBuffer buffer[MAX_AUDIO_BUFFER_CHANNELS];
//   } multiChannel;
// } mo_Audio;

// MO_API int mocha_init(mo_AudioSystem *system);
// MO_API int mocha_deinit(mo_AudioSystem *system);
/*======================
 * Audio Module
 *======================*/

MO_API Mocha* mocha_init(int flags);
MO_API int mocha_start_device(Mocha *mo);
MO_API int mocha_stop_device(Mocha *mo);
MO_API void mocha_terminate(Mocha *mo);

MO_API void mocha_set_volume(Mocha *mocha, float volume);

MO_API int mocha_get_id(Mocha *mocha, mo_AudioBuffer *buffer);
MO_API mo_AudioBuffer *mocha_get_from_id(Mocha *mocha, unsigned int id);

/***************
 * AudioBuffer
 ***************/
MO_API mo_AudioBuffer *mocha_buffer_load(Mocha *mocha, const char *filename, MO_AUDIO_USAGE_ usage);
MO_API void mocha_buffer_play(Mocha *mocha, mo_AudioBuffer *audioBuffer);
MO_API void mocha_buffer_stop(Mocha *mocha, mo_AudioBuffer *audioBuffer);
MO_API void mocha_buffer_pause(Mocha *mocha, mo_AudioBuffer *audioBuffer);
MO_API void mocha_buffer_unload(Mocha *mocha, mo_AudioBuffer *audioBuffer);

MO_API int mocha_buffer_playing(Mocha *mocha, mo_AudioBuffer *audioBuffer);
MO_API int mocha_buffer_paused(Mocha *mocha, mo_AudioBuffer *audioBuffer);

MO_API void mocha_buffer_volume(Mocha *mocha, mo_AudioBuffer *audioBuffer, float volume);

/************
 * Sound
 ************/
MO_API mo_Sound mocha_sound_load(Mocha *mocha, const char *filename, MO_AUDIO_USAGE_ usage);
MO_API void mocha_sound_unload(Mocha *mocha, mo_Sound sound);

MO_API void mocha_sound_play(Mocha *mocha, mo_Sound sound);
MO_API void mocha_sound_stop(Mocha *mocha, mo_Sound sound);
MO_API void mocha_sound_pause(Mocha *mocha, mo_Sound sound);

MO_API int mocha_sound_playing(Mocha *mocha, mo_Sound sound);
MO_API int mocha_sound_paused(Mocha *mocha, mo_Sound sound);
MO_API float mocha_sound_volume(Mocha *mocha, mo_Sound sound, float *volume);

/********************
 * Audio Data
 ********************/

MO_API void mocha_data_free(Mocha *mocha, mo_AudioData *data);

#endif // TICO_AUDIO_H