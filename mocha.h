#ifndef MOCHA_H
#define MOCHA_H

#include <stdio.h>

#define MO_API extern

#define MO_FALSE 0
#define MO_TRUE  1

#define MO_OK     0
#define MO_ERROR -1

#ifndef MO_AUDIO_FORMAT
  #define MO_AUDIO_FORMAT ma_format_f32
#endif

#ifndef MO_AUDIO_CHANNELS
  #define MO_AUDIO_CHANNELS 2
#endif

#ifndef MO_SAMPLE_RATE
  #define MO_SAMPLE_RATE 44100
#endif

#ifndef MO_BUFFER_SIZE
  #define MO_BUFFER_SIZE 4096
#endif

#ifndef MO_MAX_AUDIO_BUFFERS
  #define MO_MAX_AUDIO_BUFFERS 256
#endif

enum {
  MO_AUDIO_STREAM = 0,
  MO_AUDIO_STATIC
};

typedef struct Mocha Mocha;

typedef struct mo_audio_s mo_audio_t;
typedef int mo_id_t;

typedef unsigned int mo_uint32;
/*======================
 * Audio Module
 *======================*/
MO_API int mo_init(int flags);
MO_API void mo_quit(void);

MO_API int mo_start_device();
MO_API int mo_stop_device();

/* Audio */
MO_API mo_audio_t* mo_load_audio_from_memory(void* data, mo_uint32 size, int usage);
MO_API mo_audio_t* mo_load_audio_from_file(FILE* fp, int usage); /* TODO */
MO_API void mo_destroy_audio(mo_audio_t* audio);

MO_API void mo_audio_set_volume(mo_audio_t* audio, float volume);
MO_API void mo_audio_set_loop(mo_audio_t* audio, int loop);

MO_API mo_id_t mo_play_audio(mo_audio_t* audio);
MO_API void mo_audio_pause_all(mo_audio_t* audio);
MO_API void mo_audio_stop_all(mo_audio_t* audio);

MO_API int mo_audio_is_any_playing(mo_audio_t* audio);

/* Audio Instance */
MO_API void mo_audio_instance_play(mo_id_t id);
MO_API void mo_audio_instance_pause(mo_id_t);
MO_API void mo_audio_instance_stop(mo_id_t);
MO_API void mo_audio_instance_set_loop(mo_id_t, int loop);
MO_API void mo_audio_instance_set_volume(mo_id_t, float volume);

MO_API int mo_audio_instance_is_playing(mo_id_t);

#endif // TICO_AUDIO_H
