#include "mocha.h"

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

static Mocha _ctx;

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



Mocha* mocha_init(int flags) {
	Mocha *mo = &_ctx;
  ma_context_config ctxConfig = ma_context_config_init();
  ma_result result = ma_context_init(NULL, 0, &ctxConfig, &mo->system.ctx);
  if (result != MA_SUCCESS) {
    TRACEERR("Failed to init audio context");
    // dog_log(1, "[mocha] failed to init mocha");
    return NULL;
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
    TRACEERR("Failed to init device");
    // dog_log(1, "[mocha] failed to init mocha device");
    ma_context_uninit(&mo->system.ctx);
    // return -1;
    return NULL;
  }

  result = mocha_start_device(mo);
  if (result != MA_SUCCESS) {
    TRACEERR("Failed to start device");
    // dog_log(1, "[mocha] failed to start mocha device");
    ma_context_uninit(&mo->system.ctx);
    ma_device_uninit(&mo->system.device);
    return NULL;
  }

  if (ma_mutex_init(&mo->system.ctx, &mo->system.lock) != MA_SUCCESS) {
    TRACEERR("Failed to start mutex");
    // dog_log(1, "[mocha] failed to init mocha mutex");
    ma_device_uninit(&mo->system.device);
    ma_context_uninit(&mo->system.ctx);
    // return -1;
    return NULL;
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
  return mo;
}

int mocha_start_device(Mocha *mo)
{
  return ma_device_start(&mo->system.device);
}

int mocha_stop_device(Mocha *mo)
{
  return ma_device_stop(&mo->system.device);
}

void mocha_terminate(Mocha *mo) {
  if (mo->system.isReady) {
    ma_mutex_uninit(&mo->system.lock);
    ma_device_uninit(&mo->system.device);
    ma_context_uninit(&mo->system.ctx);
  } else {
    TRACEERR("Audio Module could not be closed, not initialized");
  }
  LOG("Audio Module terminated");
}

void mocha_set_master_volume(Mocha *mo, float volume)
{
  ma_device_set_master_volume(&mo->system.device, volume);
}

int mocha_get_id(Mocha *mo, mo_AudioBuffer *buffer)
{
  return buffer->id;
}

mo_AudioBuffer *mocha_get_from_id(Mocha *mo, unsigned int id)
{
  return &mo->multiChannel.buffer[id];
}

mo_AudioBuffer *mocha_buffer_load(Mocha *mo, const char *filename, MO_AUDIO_USAGE_ usage)
{
  int index = 0;
  int i;
  for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++)
  {
    if (!mo->multiChannel.buffer[i].loaded)
    {
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

  if (result != MA_SUCCESS)
  {
    TRACEERR("Failed to load sound '%s'", filename);
    return NULL;
  }
  audioBuffer->loaded = mo_true;
  audioBuffer->playing = mo_false;
  audioBuffer->paused = mo_true;
//   audioBuffer->usage = usage;
  audioBuffer->loop = mo_false;

  return audioBuffer;
}

void mocha_buffer_play(Mocha *mo, mo_AudioBuffer *audioBuffer)
{
  if (audioBuffer)
  {
    TRACELOG("Playing sound %d", audioBuffer->id);
    audioBuffer->playing = mo_true;
    audioBuffer->paused = mo_false;
  }
}

void mocha_buffer_stop(Mocha *mo, mo_AudioBuffer *audioBuffer)
{
  if (audioBuffer)
  {
    audioBuffer->playing = mo_false;
    ma_decoder_seek_to_pcm_frame(&audioBuffer->decoder, 0);
  }
}

void mocha_buffer_pause(Mocha *mo, mo_AudioBuffer *audioBuffer)
{
  if (audioBuffer)
  {
    audioBuffer->paused = mo_true;
    audioBuffer->playing = mo_false;
  }
}

void mocha_buffer_set_volume(Mocha *mo, mo_AudioBuffer *audioBuffer, float volume)
{
  if (audioBuffer)
    audioBuffer->volume = volume;
}

int mocha_buffer_is_playing(Mocha *mo, mo_AudioBuffer *audioBuffer)
{
  return audioBuffer->playing;
}

int mocha_buffer_is_paused(Mocha *mo, mo_AudioBuffer *audioBuffer)
{
  return audioBuffer->paused;
}

void mocha_buffer_unload(Mocha *mo, mo_AudioBuffer *audioBuffer) {
  if (audioBuffer) {
    audioBuffer->loaded = mo_false;
    audioBuffer->data->refs--;
    audioBuffer->data = NULL;
    ma_decoder_uninit(&audioBuffer->decoder);
  }
}

mo_Sound mocha_sound_load(Mocha *mo, const char *filename, MO_AUDIO_USAGE_ usage) {
  mo_Sound sound = {0};
  sound.audioBuffer = mocha_buffer_load(mo, filename, usage);
  return sound;
}
void mocha_sound_unload(Mocha *mo, mo_Sound sound) {
  mocha_buffer_unload(mo, sound.audioBuffer);
}

void mocha_sound_play(Mocha *mo, mo_Sound sound) {
  mocha_buffer_play(mo, sound.audioBuffer);
}

void mocha_sound_stop(Mocha *mo, mo_Sound sound) {
  mocha_buffer_stop(mo, sound.audioBuffer);
}

void mocha_sound_pause(Mocha *mo, mo_Sound sound) {
  mocha_buffer_pause(mo, sound.audioBuffer);
}

int mocha_sound_playing(Mocha *mo, mo_Sound sound) {
  return mocha_buffer_is_playing(mo, sound.audioBuffer);
}
int mocha_sound_paused(Mocha *mo, mo_Sound sound) {
  return mocha_buffer_is_paused(mo, sound.audioBuffer);
}

void mocha_sound_set_volume(Mocha *mo, mo_Sound sound, float volume) {
  mocha_buffer_set_volume(mo, sound.audioBuffer, volume);
}

/********************
 * Audio Data
 ********************/

void mocha_data_free(Mocha *mo, mo_AudioData *data) {
  free(data->data);
}
