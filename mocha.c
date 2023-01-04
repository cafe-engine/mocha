#include "mocha.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#define mo_true 1
#define mo_false 0

struct mo_audio_s {
    mo_uint32 id;
    char usage;
    mo_uint32 size;
    unsigned char* data;
    char loop;
    float volume, pitch;
};

typedef struct {
    unsigned short id;
    ma_decoder decoder;
    char playing, loaded;
    long int offset;
    mo_audio_t data;
} mo_audio_buffer_t;

struct Mocha {
    struct {
        ma_context ctx;
        ma_device device;
        ma_mutex lock;

        int is_ready;
    } system;
    mo_audio_buffer_t buffers[MO_MAX_AUDIO_BUFFERS];
};

static Mocha _mocha_ctx;
#define MOCHA() (&_mocha_ctx)

static int s_audio_id = 0;

static mo_uint32 s_read_and_mix_pcm_frames(mo_audio_buffer_t* buffer, float* output, mo_uint32 frames);
static void s_data_callback(ma_device* device, void* output, const void* input, mo_uint32 frames);

int mo_init(int flags) {
    Mocha *mo = MOCHA();
    ma_context_config ctxConfig = ma_context_config_init();
    ma_result result = ma_context_init(NULL, 0, &ctxConfig, &mo->system.ctx);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to init audio context\n");
        return -1;
    }

    ma_device_config devConfig = ma_device_config_init(ma_device_type_playback);
    devConfig.playback.pDeviceID = NULL;
    devConfig.playback.format = MO_AUDIO_FORMAT;
    devConfig.playback.channels = MO_AUDIO_CHANNELS;
    devConfig.sampleRate = MO_SAMPLE_RATE;
    devConfig.pUserData = MOCHA();
    devConfig.dataCallback = s_data_callback;

    result = ma_device_init(&mo->system.ctx, &devConfig, &mo->system.device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to init device\n");
        ma_context_uninit(&mo->system.ctx);
        return -1;
    }

    result = mo_start_device(mo);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to start device\n");
        ma_context_uninit(&mo->system.ctx);
        ma_device_uninit(&mo->system.device);
        return -1;
    }

    if (ma_mutex_init(&mo->system.ctx, &mo->system.lock) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start mutex\n");
        ma_device_uninit(&mo->system.device);
        ma_context_uninit(&mo->system.ctx);
        return -1;
    }

    int i;
    for (i = 0; i < MO_MAX_AUDIO_BUFFERS; i++) {
        mo_audio_buffer_t* buffer = &(mo->buffers[i]);
        memset(buffer, 0, sizeof(*buffer));
        buffer->playing = mo_false;
        buffer->data.volume = 1.f;
        buffer->data.pitch = 1.f;
        buffer->loaded = mo_false;
        buffer->data.loop = mo_false;
    }
    mo->system.is_ready = mo_true;
    return 0;
}

int mo_start_device() {
    Mocha *mo = MOCHA();
    return ma_device_start(&(mo->system.device));
}

int mo_stop_device() {
    Mocha *mo = MOCHA();
    return ma_device_stop(&(mo->system.device));
}

void mo_quit(void) {
    Mocha *mo = MOCHA();
    if (mo->system.is_ready) {
        ma_mutex_uninit(&mo->system.lock);
        ma_device_uninit(&mo->system.device);
        ma_context_uninit(&mo->system.ctx);
    } else {
        fprintf(stderr, "Audio Module could not be closed, not initialized");
    }
}

mo_audio_t* mo_load_audio_from_memory(void* data, mo_uint32 size, int usage) {
    mo_audio_t* audio = NULL;
    switch (usage) {
        case MO_AUDIO_STREAM:
        {
            audio = malloc(sizeof(*audio));
            audio->data = data;
            audio->size = size;
        }
        break;
        case MO_AUDIO_STATIC:
        {
            ma_decoder_config dec_config = ma_decoder_config_init(MO_AUDIO_FORMAT, MO_AUDIO_CHANNELS, MO_SAMPLE_RATE);
            ma_uint64 frame_count_out;
            void* dec_data;
            ma_result result = ma_decode_memory(data, size, &dec_config, &frame_count_out, &dec_data);
            if (result != MA_SUCCESS) return audio;
            audio = malloc(sizeof(*audio));
            audio->data = dec_data;
            audio->size = frame_count_out;
        }
        break;
        default:
            return audio;
    }

    audio->id = ++s_audio_id;
    audio->usage = usage;
    audio->loop = 0;
    audio->volume = 1.f;
    audio->pitch = 0.f;

    return audio;
}

mo_audio_t* mo_load_audio_from_file(FILE* fp, int usage) {
    mo_audio_t* audio = NULL;
    if (!fp) return audio;
    switch (usage) {
        case MO_AUDIO_STREAM: break;
        case MO_AUDIO_STATIC: break;
        default: return audio;
    }
    return audio;
}

void mo_destroy_audio(mo_audio_t* audio) {
    if (!audio) return;
    if (audio->usage == MO_AUDIO_STATIC) {
        free(audio->data);
    }
    free(audio);
}

mo_id_t mo_play_audio(mo_audio_t* audio) {
    if (!audio) return -1;
    mo_id_t id = 0;
    int i;
    for (i = 0; i < MO_MAX_AUDIO_BUFFERS; i++) {
        if (!(MOCHA()->buffers[i].loaded)) break;
    }
    id = i+1;
    mo_audio_buffer_t* buffer = &(MOCHA()->buffers[i]);
    ma_decoder_config dec_config = ma_decoder_config_init(MO_AUDIO_FORMAT, MO_AUDIO_CHANNELS, MO_SAMPLE_RATE);
    memcpy(&(buffer->data), audio, sizeof(mo_audio_t));
    ma_result result = MA_SUCCESS;
    buffer->offset = 0;
    if (audio->usage == MO_AUDIO_STREAM) {
        result = ma_decoder_init_memory(audio->data, audio->size, &dec_config, &(buffer->decoder));
        ma_decoder_seek_to_pcm_frame(&(buffer->decoder), 0);
    }

    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to load sound: %s\n", ma_result_description(result));
        return -1;
    }

    buffer->loaded = 1;
    buffer->playing = 1;
    buffer->id = audio->id;
    return id;
}

void mo_audio_pause_all(mo_audio_t* audio) {
    if (!audio) return;
    mo_audio_buffer_t* b = NULL;
    for (int i = 0; i < MO_MAX_AUDIO_BUFFERS; i++) {
        b = &(MOCHA()->buffers[i]);
        if (b->id == audio->id && b->loaded && b->playing) b->playing = 0;
    }
}

void mo_audio_stop_all(mo_audio_t* audio) {
    if (!audio) return;
    mo_audio_buffer_t* b = NULL;
    for (int i = 0; i < MO_MAX_AUDIO_BUFFERS; i++) {
        b = &(MOCHA()->buffers[i]);
        if (b->id == audio->id && b->loaded) b->loaded = 0;
    }
}

int mo_audio_is_any_playing(mo_audio_t* audio) {
    if (!audio) return 0;
    mo_audio_buffer_t* b = NULL;
    for (int i = 0; i < MO_MAX_AUDIO_BUFFERS; i++) {
        b = &(MOCHA()->buffers[i]);
        if (b->id == audio->id && b->loaded && b->playing) return 1;
    }
    return 0;
}

/* Audio Instance */

#define GET_BUFFER(id)\
mo_audio_buffer_t* buffer = &(MOCHA()->buffers[(id)])

void mo_audio_instance_play(mo_id_t id) {
    if (id <= 0 || id > MO_MAX_AUDIO_BUFFERS) return;
    GET_BUFFER(id-1);
    if (buffer->loaded) buffer->playing = 1;
}

void mo_audio_instance_pause(mo_id_t id) {
    if (id <= 0 || id > MO_MAX_AUDIO_BUFFERS) return;
    GET_BUFFER(id-1);
    if (buffer->loaded && buffer->playing) buffer->playing = 0;
}

void mo_audio_instance_stop(mo_id_t id) {
    if (id <= 0 || id > MO_MAX_AUDIO_BUFFERS) return;
    GET_BUFFER(id-1);
    if (buffer->loaded) buffer->loaded = 0;
}

void mo_audio_instance_set_volume(mo_id_t id, float volume) {
    if (id <= 0 || id > MO_MAX_AUDIO_BUFFERS) return;
    GET_BUFFER(id-1);
    buffer->data.volume = volume;
}

int mo_audio_instance_is_playing(mo_id_t id) {
    if (id <= 0 || id > MO_MAX_AUDIO_BUFFERS) return 0;
    GET_BUFFER(id-1);
    return (buffer->loaded && buffer->playing);
}

/* Static */
mo_uint32 s_read_and_mix_pcm_frames(mo_audio_buffer_t* buffer, float* output, mo_uint32 frames) {
    float temp[MO_BUFFER_SIZE];
    mo_uint32 temp_cap_in_frames = ma_countof(temp) / MO_AUDIO_CHANNELS;
    mo_uint32 total_frames_read = 0;
    ma_decoder* decoder = &buffer->decoder;
    mo_audio_t* data = &(buffer->data);
    float volume = data->volume;
    float size = data->size * ma_get_bytes_per_frame(MO_AUDIO_FORMAT, MO_AUDIO_CHANNELS);

    while (total_frames_read < frames) {
        mo_uint32 sample;
        mo_uint32 frames_read_this_iteration;
        mo_uint32 total_frames_remaining = frames - total_frames_read;
        mo_uint32 frames_to_read_this_iteration = temp_cap_in_frames;
        if (frames_to_read_this_iteration > total_frames_remaining) {
            frames_to_read_this_iteration = total_frames_remaining;
        }

        if (data->usage == MO_AUDIO_STREAM) {
            frames_read_this_iteration = (mo_uint32)ma_decoder_read_pcm_frames(decoder, temp, frames_to_read_this_iteration);
        }
        else {
            frames_read_this_iteration = frames_to_read_this_iteration;
            mo_uint32 aux = frames_to_read_this_iteration * ma_get_bytes_per_frame(MO_AUDIO_FORMAT, MO_AUDIO_CHANNELS);
            memcpy(temp, data->data + buffer->offset, aux);
            if (buffer->offset > size) frames_read_this_iteration = 0;
            buffer->offset += aux;
        }

        if (frames_read_this_iteration == 0) {
            break;
        }

        for (sample = 0; sample < frames_read_this_iteration * MO_AUDIO_CHANNELS; ++sample) {
            output[total_frames_read * MO_AUDIO_CHANNELS + sample] += temp[sample] * volume;
        }

        total_frames_read += frames_read_this_iteration;

        if (frames_read_this_iteration < frames_to_read_this_iteration) {
            break;
        }
    }

    return total_frames_read;
}

void s_data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    float *fOutput = (float *)pOutput;
    Mocha *mo = pDevice->pUserData;

    ma_mutex_lock(&mo->system.lock);
    int i;
    for (i = 0; i < MO_MAX_AUDIO_BUFFERS; i++) {
        mo_audio_buffer_t *buffer = &(mo->buffers[i]);
        if (buffer->playing && buffer->loaded) {
            ma_uint32 framesRead = s_read_and_mix_pcm_frames(buffer, fOutput, frameCount);
            if (framesRead < frameCount) {
                ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
                buffer->offset = 0;
                if (!buffer->data.loop) {
                    buffer->playing = mo_false;
                }
            }
        }
    }
    ma_mutex_unlock(&mo->system.lock);
    (void)pInput;
}