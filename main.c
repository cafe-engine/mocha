#include "mocha.h"

int main(int argc, char ** argv) {
    mocha_init(0);

    FILE *fp = fopen("music.wav", "r");
    fseek(fp, 0, SEEK_END);
    unsigned int sz = ftell(fp);
    fseek(fp, 0, SEEK_SET); 

    void *data = malloc(sz);
    fread(data, sz, 1, fp);

    fclose(fp);

    mo_AudioBuffer *buf = mocha_buffer(data, sz, MO_AUDIO_STREAM);
    mocha_buffer_play(buf);

    // mocha_sound_stop(ctx, snd);
    while (mocha_buffer_playing(buf));

    mocha_terminate();

    return 0;
}
