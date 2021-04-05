#include "mocha.h"

#define CSTAR_IMPLEMENTATION
#include "cstar.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#define mo_true 1
#define mo_false 0

struct mo_buffer_s {
    unsigned short id;
    ma_decoder decoder;
    float volume, pitch;

    int playing, paused;
    int loop, loaded;
    mo_uint32 offset;

    // mo_data_t data;
    int usage;
    union { void *data, *fp; };
    mo_uint32 size;
};

typedef unsigned int mo_sound_t;

struct mo_AudioBuffer {
  ma_decoder decoder;
  unsigned char id;

  float volume;
  float pitch;

  int playing;
  int paused;
  int loop;
  int loaded;

//   MO_AUDIO_USAGE usage;

//   unsigned char *data;
//   size_t dataSize;

  mo_AudioData *data;

  size_t currentReadPos;
};

struct Mocha {
  struct {
    ma_context ctx;
    ma_device device;
    ma_mutex lock;

    int isReady;
  } system;

  mo_AudioData data[MAX_AUDIO_DATA];
  mo_AudioBuffer channels[MAX_AUDIO_BUFFER_CHANNELS];

  struct {
    mo_AudioBuffer buffer[MAX_AUDIO_BUFFER_CHANNELS];
  } multiChannel;
};

static Mocha _mocha_ctx;
#define mocha() (&_mocha_ctx)

static char* _readfile(const char *filename, size_t *size) {
    FILE *fp;
    char *buffer;
    int sz = 0;

    fp = fopen(filename, "rb");

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = malloc(sz+1);
    fread(buffer, 1, sz, fp);
    if (size) *size = sz;

    fclose(fp);

    return buffer;
}

// static mo_Audio audio = {0};

static ma_uint32 _read_and_mix_pcm_frames(mo_AudioBuffer *audioBuffer, float *pOutputF32, ma_uint32 frameCount) {

  float temp[4096];
  ma_uint32 tempCapInFrames = ma_countof(temp) / AUDIO_DEVICE_CHANNELS;
  ma_uint32 totalFramesRead = 0;
  ma_decoder *pDecoder = &audioBuffer->decoder;
  float volume = audioBuffer->volume;

  while (totalFramesRead < frameCount) {
    ma_uint32 iSample;
    ma_uint32 framesReadThisIteration;
    ma_uint32 totalFramesRemaining = frameCount - totalFramesRead;
    ma_uint32 framesToReadThisIteration = tempCapInFrames;
    if (framesToReadThisIteration > totalFramesRemaining) {
      framesToReadThisIteration = totalFramesRemaining;
    }

    if (audioBuffer->data->usage == MO_AUDIO_STREAM) {
      framesReadThisIteration = (ma_uint32)ma_decoder_read_pcm_frames(pDecoder, temp, framesToReadThisIteration);
    } else {
      framesReadThisIteration = framesToReadThisIteration;
      memcpy(temp, audioBuffer->data + audioBuffer->currentReadPos, framesToReadThisIteration);
      audioBuffer->currentReadPos += framesReadThisIteration;
    }


    if (framesReadThisIteration == 0) {
      break;
    }

    for (iSample = 0; iSample < framesReadThisIteration * AUDIO_DEVICE_CHANNELS; ++iSample) {
      pOutputF32[totalFramesRead * AUDIO_DEVICE_CHANNELS + iSample] += temp[iSample] * volume;
    }

    totalFramesRead += framesReadThisIteration;

    if (framesReadThisIteration < framesToReadThisIteration) {
      break;
    }
  }

  return totalFramesRead;
}

static void _data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
  float *fOutput = (float *)pOutput;
  Mocha *mo = pDevice->pUserData;

  ma_mutex_lock(&mo->system.lock);
  int i;
  for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
    mo_AudioBuffer *audioBuffer = &mo->multiChannel.buffer[i];
    if (audioBuffer->playing && audioBuffer->loaded && !audioBuffer->paused) {
      ma_uint32 framesRead = _read_and_mix_pcm_frames(audioBuffer, fOutput, frameCount);
      if (framesRead < frameCount) {
        if (audioBuffer->loop) {
          ma_decoder_seek_to_pcm_frame(&audioBuffer->decoder, 0);
          audioBuffer->currentReadPos = 0;
        } else {
          audioBuffer->playing = mo_false;
        }
      }
    }
  }
  ma_mutex_unlock(&mo->system.lock);

  (void)pInput;
}



int mocha_init(int flags) {
    Mocha *mo = mocha();
    ma_context_config ctxConfig = ma_context_config_init();
    ma_result result = ma_context_init(NULL, 0, &ctxConfig, &mo->system.ctx);
    if (result != MA_SUCCESS) {
        cst_error("Failed to init audio context");
        // dog_log(1, "[mocha] failed to init mocha");
        return 0;
    }

    ma_device_config devConfig = ma_device_config_init(ma_device_type_playback);
    devConfig.playback.pDeviceID = NULL;
    devConfig.playback.format = AUDIO_DEVICE_FORMAT;
    devConfig.playback.channels = AUDIO_DEVICE_CHANNELS;
    devConfig.sampleRate = AUDIO_DEVICE_SAMPLE_RATE;
    devConfig.pUserData = mo;
    devConfig.dataCallback = _data_callback;

  result = ma_device_init(&mo->system.ctx, &devConfig, &mo->system.device);
  if (result != MA_SUCCESS) {
    cst_error("Failed to init device");
    // dog_log(1, "[mocha] failed to init mocha device");
    ma_context_uninit(&mo->system.ctx);
    // return -1;
    return 0;
  }

  result = mocha_start_device(mo);
  if (result != MA_SUCCESS) {
    cst_error("Failed to start device");
    // dog_log(1, "[mocha] failed to start mocha device");
    ma_context_uninit(&mo->system.ctx);
    ma_device_uninit(&mo->system.device);
    return 0;
  }

  if (ma_mutex_init(&mo->system.ctx, &mo->system.lock) != MA_SUCCESS) {
    cst_error("Failed to start mutex");
    // dog_log(1, "[mocha] failed to init mocha mutex");
    ma_device_uninit(&mo->system.device);
    ma_context_uninit(&mo->system.ctx);
    // return -1;
    return 0;
  }

  int i;
  for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
    mo_AudioBuffer *buffer = &mo->multiChannel.buffer[i];
    buffer->playing = mo_false;
    buffer->volume = 1.f;
    buffer->pitch = 1.f;
    buffer->loaded = mo_false;
    buffer->paused = mo_false;
    buffer->loop = mo_false;
    buffer->id = i;
  }

  mo->system.isReady = mo_true;

  // LOG("Audio Module initiated");
  return 1;
}

int mocha_start_device() {
    Mocha *mo = mocha();
    return ma_device_start(&mo->system.device);
}

int mocha_stop_device() {
    Mocha *mo = mocha();
    return ma_device_stop(&mo->system.device);
}

void mocha_terminate() {
    Mocha *mo = mocha();
    if (mo->system.isReady) {
        ma_mutex_uninit(&mo->system.lock);
        ma_device_uninit(&mo->system.device);
        ma_context_uninit(&mo->system.ctx);
    } else {
        cst_error("Audio Module could not be closed, not initialized");
    }
    cst_log("Audio Module terminated");
}

void mocha_set_master_volume(float volume) {
    Mocha *mo = mocha();
    ma_device_set_master_volume(&mo->system.device, volume);
}

int mocha_get_id(mo_AudioBuffer *buffer) {
    return buffer->id;
}

mo_AudioBuffer *mocha_get_from_id(unsigned int id) {
    Mocha *mo = mocha();
    return &mo->multiChannel.buffer[id];
}

mo_AudioBuffer *mocha_buffer(void *data, long size, int usage) {
    Mocha *mo = mocha();
    int index = 0;
    int i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        if (!mo->multiChannel.buffer[i].loaded) {
            index = i;
            break;
        }
    }

    mo_AudioBuffer *buff = &mo->multiChannel.buffer[index];
    ma_decoder_config decoderConfig = ma_decoder_config_init(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, AUDIO_DEVICE_SAMPLE_RATE);
    ma_result result = 0;
    buff->currentReadPos = 0;
    if (usage == MO_AUDIO_STREAM) {
        mo_AudioData *aData = NULL;
        aData = malloc(sizeof(*aData));
        aData->data = (unsigned char*)data;
        aData->usage = usage;
        aData->size = size;
        result = ma_decoder_init_memory(aData->data, aData->size, &decoderConfig, &buff->decoder);
        buff->data = aData;
        // mocha_resources_add_sound(filename, aData);
        aData->refs++;
    } else {
        ma_uint64 pFrameCountOut;
        void *ppData;
        result = ma_decode_memory(data, size, &decoderConfig, &pFrameCountOut, &ppData);
        buff->data = ppData;
        free(data);
    }

    if (result != MA_SUCCESS) {
        cst_error("Failed to load sound");
        return NULL;
     }
    buff->loaded = mo_true;
    buff->playing = mo_false;
    buff->paused = mo_true;
    //   audioBuffer->usage = usage;
    buff->loop = mo_false;

    return buff;
}

mo_AudioBuffer *mocha_buffer_load(const char *filename, MO_AUDIO_USAGE_ usage) {
    Mocha *mo = mocha();
    int index = 0;
    int i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        if (!mo->multiChannel.buffer[i].loaded) {
            index = i;
            break;
        }
    }
    mo_AudioBuffer *audioBuffer = &mo->multiChannel.buffer[index];
    ma_decoder_config decoderConfig = ma_decoder_config_init(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, AUDIO_DEVICE_SAMPLE_RATE);
    ma_result result = 0;
    audioBuffer->currentReadPos = 0;

    if (usage == MO_AUDIO_STREAM) {
        // mo_AudioData *aData = mocha_resources_get_sound(filename);
  	mo_AudioData *aData = NULL;
        if (aData && aData->usage == usage) {
            audioBuffer->data = aData;
        } else {
            aData = malloc(sizeof(*aData));
            aData->data = (unsigned char*)_readfile(filename, &aData->size);
            aData->usage = usage;
        }

        result = ma_decoder_init_memory(aData->data, aData->size, &decoderConfig, &audioBuffer->decoder);
        audioBuffer->data = aData;
        // mocha_resources_add_sound(filename, aData);
        aData->refs++;
    } else {
        size_t size;
        char *data = _readfile(filename, &size);
        ma_uint64 pFrameCountOut;
        void *ppData;
        result = ma_decode_memory(data, size, &decoderConfig, &pFrameCountOut, &ppData);
        audioBuffer->data = ppData;
        free(data);
    }

    if (result != MA_SUCCESS) {
        cst_error("Failed to load sound '%s'", filename);
        return NULL;
     }
    audioBuffer->loaded = mo_true;
    audioBuffer->playing = mo_false;
    audioBuffer->paused = mo_true;
    //   audioBuffer->usage = usage;
    audioBuffer->loop = mo_false;

    return audioBuffer;
}

void mocha_buffer_play(mo_AudioBuffer *audioBuffer) {
  if (audioBuffer) {
    audioBuffer->playing = mo_true;
    audioBuffer->paused = mo_false;
  }
}

void mocha_buffer_stop(mo_AudioBuffer *audioBuffer) {
  if (audioBuffer) {
    audioBuffer->playing = mo_false;
    ma_decoder_seek_to_pcm_frame(&audioBuffer->decoder, 0);
  }
}

void mocha_buffer_pause(mo_AudioBuffer *audioBuffer) {
  if (audioBuffer) {
    audioBuffer->paused = mo_true;
    audioBuffer->playing = mo_false;
  }
}

void mocha_buffer_set_volume(mo_AudioBuffer *audioBuffer, float volume) {
  if (audioBuffer) audioBuffer->volume = volume;
}

int mocha_buffer_playing(mo_AudioBuffer *audioBuffer) {
  return audioBuffer->playing;
}

int mocha_buffer_paused(mo_AudioBuffer *audioBuffer) {
  return audioBuffer->paused;
}

void mocha_buffer_unload(mo_AudioBuffer *audioBuffer) {
  if (audioBuffer) {
    audioBuffer->loaded = mo_false;
    audioBuffer->data->refs--;
    audioBuffer->data = NULL;
    ma_decoder_uninit(&audioBuffer->decoder);
  }
}

mo_Sound mocha_sound_load(const char *filename, MO_AUDIO_USAGE_ usage) {
  mo_Sound sound = {0};
  sound.audioBuffer = mocha_buffer_load(filename, usage);
  return sound;
}

void mocha_sound_unload(mo_Sound sound) {
  mocha_buffer_unload(sound.audioBuffer);
}

void mocha_sound_play(mo_Sound sound) {
  mocha_buffer_play(sound.audioBuffer);
}

void mocha_sound_stop(mo_Sound sound) {
  mocha_buffer_stop(sound.audioBuffer);
}

void mocha_sound_pause(mo_Sound sound) {
  mocha_buffer_pause(sound.audioBuffer);
}

int mocha_sound_playing(mo_Sound sound) {
  return mocha_buffer_playing(sound.audioBuffer);
}
int mocha_sound_paused(mo_Sound sound) {
  return mocha_buffer_paused(sound.audioBuffer);
}

void mocha_sound_set_volume(mo_Sound sound, float volume) {
  mocha_buffer_set_volume(sound.audioBuffer, volume);
}

/********************
 * Audio Data
 ********************/

void mocha_data_free(mo_AudioData *data) {
  free(data->data);
}
