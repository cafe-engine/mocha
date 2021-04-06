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
#define MO_TYPE_MASK 0x10

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

typedef struct mo_audio_s mo_audio_t;
typedef struct mo_wave_s mo_wave_t;
typedef unsigned int mo_uint32;

struct mo_data_s {
  int usage;
  union {
    void *data;
    void *fp;
  };
  mo_uint32 size;
};

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

// mo_init(int flags);
// mo_deinit();

// mo_set_volume(float volume);
// mo_start();
// mo_stop();

// mo_buffer_t mo_buffer_from_data(void *data, mo_uint32 size, int usage);
// mo_buffer_t mo_buffer_from_file(const char *filename, int usage);
// int mo_buffer_play(mo_buffer_t *buff);
// int mo_buffer_stop(mo_buffer_t *buff);
// int mo_buffer_pause(mo_buffer_t *buff);
// int mo_buffer_deinit(mo_buffer_t *buff);

/*======================
 * Audio Module
 *======================*/
MO_API int mo_init(int flags);
MO_API int mo_deinit();

MO_API int mo_start_device();
MO_API int mo_stop_device();

MO_API mo_audio_t* mo_audio(void *data, mo_uint32 size, int usage);
MO_API mo_audio_t* mo_audio_load(const char *filename, int usage);
MO_API int mo_audio_destroy(mo_audio_t *audio);

MO_API float mo_volume(mo_audio_t *audio, float volume);
MO_API int mo_play(mo_audio_t *audio);
MO_API int mo_pause(mo_audio_t *audio);
MO_API int mo_stop(mo_audio_t *audio);

MO_API int mo_is_playing(mo_audio_t *audio);

MO_API int mocha_init(int flags);
MO_API int mocha_start_device();
MO_API int mocha_stop_device();
MO_API void mocha_terminate();

MO_API void mocha_set_volume(float volume);

MO_API int mocha_get_id(mo_AudioBuffer *buffer);
MO_API mo_AudioBuffer *mocha_get_from_id(unsigned int id);

/***************
 * AudioBuffer
 ***************/
MO_API mo_AudioBuffer *mocha_buffer(void *data, long size, int audio_usage);
MO_API mo_AudioBuffer *mocha_buffer_load(const char *filename, MO_AUDIO_USAGE_ usage);
MO_API void mocha_buffer_play(mo_AudioBuffer *audioBuffer);
MO_API void mocha_buffer_stop(mo_AudioBuffer *audioBuffer);
MO_API void mocha_buffer_pause(mo_AudioBuffer *audioBuffer);
MO_API void mocha_buffer_unload(mo_AudioBuffer *audioBuffer);

MO_API int mocha_buffer_playing(mo_AudioBuffer *audioBuffer);
MO_API int mocha_buffer_paused(mo_AudioBuffer *audioBuffer);

MO_API void mocha_buffer_volume(mo_AudioBuffer *audioBuffer, float volume);

/************
 * Sound
 ************/
MO_API mo_Sound mocha_sound_load(const char *filename, MO_AUDIO_USAGE_ usage);
MO_API void mocha_sound_unload(mo_Sound sound);

MO_API void mocha_sound_play(mo_Sound sound);
MO_API void mocha_sound_stop(mo_Sound sound);
MO_API void mocha_sound_pause(mo_Sound sound);

MO_API int mocha_sound_playing(mo_Sound sound);
MO_API int mocha_sound_paused(mo_Sound sound);
MO_API float mocha_sound_volume(mo_Sound sound, float *volume);

/********************
 * Audio Data
 ********************/

MO_API void mocha_data_free(mo_AudioData *data);

#endif // TICO_AUDIO_H
