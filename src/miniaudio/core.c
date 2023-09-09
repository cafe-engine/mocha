#include <stdio.h>

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

#ifndef AUDIO_FORMAT
    #define AUDIO_FORMAT ma_format_f32
#endif

#ifndef AUDIO_CHANNELS
    #define AUDIO_CHANNELS 2
#endif

#ifndef AUDIO_SAMPLE_RATE
    #define AUDIO_SAMPLE_RATE 44100
#endif

#ifndef AUDIO_BUFFER_SIZE
    #define AUDIO_BUFFER_SIZE 4096
#endif

#define DEFAULT_DATA_BANK_SIZE 64
#define DEFAULT_BUFFERS_SIZE 64

enum {
    AUDIO_STREAM = 0,
    AUDIO_STATIC
};

struct AudioParams {
    float volume;
    float pitch;
    char loop;
};

struct AudioData {
    int usage;
    unsigned int size;
    void* data;  
};

struct AudioBuffer {
    unsigned int id;
    char playing, loaded;
    unsigned int offset;
    struct AudioParams params;
    struct AudioData data;
    ma_decoder decoder;
};

static void core_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);

typedef struct Stack Stack;
struct Stack {
    int top;
    int count;
    int* data;
};

static void init_stack(Stack* s, int size) {
    s->top = 0;
    s->count = size;
    s->data = malloc(size * sizeof(int));
}

static void deinit_stack(Stack* s) { free(s->data); }

static void stack_grow(Stack* s) {
    s->count *= 2;
    s->data = realloc(s->data, sizeof(int) * s->count);
}

static void stack_push(Stack* s, int v) {
    if (s->top >= s->count) {
        fprintf(stderr, "Stack overflow\n");
        return;
    }
    s->data[s->top++] = v;
}

static int stack_pop(Stack* s) {
    if (s->top <= 0) {
        fprintf(stderr, "Stack underflow\n");
        return -1;
    }
    return s->data[--(s->top)];
}

static int stack_top(Stack* s) {
    return s->data[s->top-1];
}

struct Mocha {
    ma_context ctx;
    ma_device device;
    ma_mutex lock;
    char is_ready;

    Stack available_data;
    struct AudioData* data_bank;

    Stack available_buffers;
    struct AudioBuffer* buffers;
};

static struct Mocha _mocha;

int core_init(void) {
    ma_context_config ctx_config = ma_context_config_init();
    ma_result result = ma_context_init(NULL, 0, &ctx_config, &(_mocha.ctx));
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to init audio context\n");
        return -1;
    }

    ma_device_config dev_config = ma_device_config_init(ma_device_type_playback);
    dev_config.playback.pDeviceID = NULL;
    dev_config.playback.format = AUDIO_FORMAT;
    dev_config.playback.channels = AUDIO_CHANNELS;
    dev_config.sampleRate = AUDIO_SAMPLE_RATE;
    dev_config.pUserData = &_mocha;
    dev_config.dataCallback = core_data_callback;

    result = ma_device_init(&(_mocha.ctx), &dev_config, &(_mocha.device));
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to init device\n");
        ma_context_uninit(&(_mocha.ctx));
        return -1;
    }

    if (ma_mutex_init(&(_mocha.ctx), &(_mocha.lock)) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start mutex\n");
        ma_device_uninit(&(_mocha.device));
        ma_context_uninit(&(_mocha.ctx));
        return -1;
    }

    init_stack(&(_mocha.available_data), DEFAULT_DATA_BANK_SIZE);
    _mocha.data_bank = malloc(sizeof(struct AudioData) * DEFAULT_DATA_BANK_SIZE);
    if (!_mocha.data_bank) {
        fprintf(stderr, "Failed to alloc memory for audio data bank\n");
        return -1;
    }
    memset(_mocha.data_bank, 0, sizeof(struct AudioData) * DEFAULT_DATA_BANK_SIZE);

    init_stack(&(_mocha.available_buffers), DEFAULT_BUFFERS_SIZE);
    _mocha.buffers = malloc(sizeof(struct AudioBuffer) * DEFAULT_BUFFERS_SIZE);
    if (!_mocha.buffers) {
        fprintf(stderr, "Failed to alloc memory for audio buffers\n");
        return -1;
    }
    memset(_mocha.buffers, 0, sizeof(struct AudioBuffer) * DEFAULT_BUFFERS_SIZE);

    Stack* s = &(_mocha.available_data);
    for (int i = 0; i < DEFAULT_DATA_BANK_SIZE; i++) {
        stack_push(s, DEFAULT_DATA_BANK_SIZE - i - 1);
    }

    s = &(_mocha.available_buffers);
    for (int i = 0; i < DEFAULT_BUFFERS_SIZE; i++) {
        stack_push(s, DEFAULT_BUFFERS_SIZE - i);
    }

    ma_device_start(&(_mocha.device));
    _mocha.is_ready = 1;
    return 0;
}

int core_start_device() {
    return ma_device_start(&(_mocha.device));
}

int core_stop_device() {
    return ma_device_stop(&(_mocha.device));
}

void core_quit() {
    if (_mocha.is_ready) {
        core_stop_device();
        ma_mutex_uninit(&(_mocha.lock));
        ma_device_uninit(&(_mocha.device));
        ma_context_uninit(&(_mocha.ctx));

        for (int i = 0; i < _mocha.available_data.count; i++) {
            struct AudioData* data = &(_mocha.data_bank[i]);
            if (data->data) free(data->data);
        }
        deinit_stack(&(_mocha.available_data));
        deinit_stack(&(_mocha.available_buffers));
    } else {
        fprintf(stderr, "Mocha could not be closed, not initialized\n");
    }
}

int core_new_data_from_memory(void* data, ma_uint32 size, int usage) {
    if (usage != AUDIO_STATIC && usage != AUDIO_STREAM) return -1;
    int id = stack_pop(&(_mocha.available_data));
    struct AudioData* dt = &(_mocha.data_bank[id]);
    switch (usage) {
        case AUDIO_STREAM:
        {
            void* copy_data = malloc(size);
            memcpy(copy_data, data, size);
            dt->size = size;
            dt->data = copy_data;
        }
        break;
        case AUDIO_STATIC:
        {
            ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_SAMPLE_RATE);
            ma_uint64 frame_count_out;
            void* dec_data;
            ma_result result = ma_decode_memory(data, size, &dec_config, &frame_count_out, &dec_data);
            if (result != MA_SUCCESS) return -1;
            dt->size = frame_count_out;
            dt->data = dec_data;
        }
        break;
    }
    dt->usage = usage;
    return id;
}

void core_release_data(int id) {
    Stack* pool = &(_mocha.available_data);
    if (id < 0 || id >= _mocha.available_data.count) return;
    struct AudioData* data = &(_mocha.data_bank[id]);
    if (data->size == 0 || data->data == NULL) return;
    data->size = 0;
    free(data->data);
    data->data = NULL;
    stack_push(pool, id);
}

struct AudioBuffer* core_get_buffer(int id) {
    if (id < 1) return NULL;
    return &(_mocha.buffers[id-1]);
}

int core_get_available_buffer() {
    int id = stack_top(&(_mocha.available_buffers));
    return id;
}

void core_setup_buffer(int id, int data_id, const struct AudioParams* params) {
    struct AudioBuffer* buffer = &(_mocha.buffers[id-1]);
    struct AudioData* data = &(_mocha.data_bank[data_id]);

    ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_SAMPLE_RATE);
    memcpy(&(buffer->data), data, sizeof(struct AudioData));
    ma_result result = MA_SUCCESS;
    buffer->offset = 0;
    if (data->usage == AUDIO_STREAM) {
        result = ma_decoder_init_memory(data->data, data->size, &dec_config, &(buffer->decoder));
        ma_decoder_seek_to_pcm_frame(&(buffer->decoder), 0);
    }

    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to load sound: %s\n", ma_result_description(result));
        return;
    }

    memcpy(&(buffer->params), params, sizeof(*params));
    buffer->id = data_id;
    buffer->loaded = 1;
    buffer->playing = 0;
}

int core_setup_free_buffer(int data_id, struct AudioParams* params) {
    int id = stack_pop(&(_mocha.available_buffers));
    core_setup_buffer(id, data_id, params);
    return id;
}

void core_init_data_from_memory(struct AudioData* audio, void* data, ma_uint32 size, int usage) {
    switch (usage) {
        case AUDIO_STREAM:
        {
            audio->data = malloc(size);
            audio->size = size;
            audio->usage = usage;
            memcpy(audio->data, data, size);
        }
        break;
        case AUDIO_STATIC:
        {
            ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_SAMPLE_RATE);
            ma_uint64 frame_count_out;
            void* dec_data;
            ma_result result = ma_decode_memory(data, size, &dec_config, &frame_count_out, &dec_data);
            if (result != MA_SUCCESS) return;
            audio = malloc(sizeof(*audio));
            audio->data = dec_data;
            audio->size = frame_count_out;
        }
        break;
        default: return;
    }
    audio->usage = usage;
}

unsigned int core_read_and_mix_pcm_frames(struct AudioBuffer* buffer, float* output, unsigned int frames) {
    float temp[AUDIO_BUFFER_SIZE];
    ma_uint32 temp_cap_in_frames = ma_countof(temp) / AUDIO_CHANNELS;
    ma_uint32 total_frames_read = 0;
    ma_decoder* decoder = &buffer->decoder;
    struct AudioData* data = &(buffer->data);
    float volume = buffer->params.volume;
    float size = data->size * ma_get_bytes_per_frame(AUDIO_FORMAT, AUDIO_CHANNELS);

    while (total_frames_read < frames) {
        ma_uint32 sample;
        ma_uint32 frames_read_this_iteration;
        ma_uint32 total_frames_remaining = frames - total_frames_read;
        ma_uint32 frames_to_read_this_iteration = temp_cap_in_frames;
        if (frames_to_read_this_iteration > total_frames_remaining) {
            frames_to_read_this_iteration = total_frames_remaining;
        }

        if (data->usage == AUDIO_STREAM) {
            frames_read_this_iteration = (ma_uint32)ma_decoder_read_pcm_frames(decoder, temp, frames_to_read_this_iteration);
        }
        else {
            frames_read_this_iteration = frames_to_read_this_iteration;
            ma_uint32 aux = frames_to_read_this_iteration * ma_get_bytes_per_frame(AUDIO_FORMAT, AUDIO_CHANNELS);
            char* dt = data->data;
            memcpy(temp, dt + buffer->offset, aux);
            if (buffer->offset > size) frames_read_this_iteration = 0;
            buffer->offset += aux;
        }

        if (frames_read_this_iteration == 0) {
            break;
        }

        for (sample = 0; sample < frames_read_this_iteration * AUDIO_CHANNELS; ++sample) {
            output[total_frames_read * AUDIO_CHANNELS + sample] += temp[sample] * volume;
        }

        total_frames_read += frames_read_this_iteration;

        if (frames_read_this_iteration < frames_to_read_this_iteration) {
            break;
        }
    }

    return total_frames_read;
}

void core_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
    float* out = (float*)output;
    struct Mocha* mo = device->pUserData;

    ma_mutex_lock(&(mo->lock));
    int i;
    for (i = 0; i < mo->available_buffers.count; i++) {
        struct AudioBuffer *buffer = &(mo->buffers[i]);
        // fprintf(stderr, "Buffer[%d], %d %d\n", i, buffer->playing, buffer->loaded);
        if (buffer->playing && buffer->loaded) {
            ma_uint32 frames_read = core_read_and_mix_pcm_frames(buffer, output, frame_count);
            if (frames_read < frame_count) {
                ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
                buffer->offset = 0;
                buffer->playing = buffer->params.loop;
                buffer->loaded = buffer->params.loop;
                if (!(buffer->params.loop)) {
                    buffer->playing = 0;
                }
            }
        }
    }
    ma_mutex_unlock(&(mo->lock));
    (void)input;
}
