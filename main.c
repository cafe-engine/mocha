#include "mocha.h"

int main(int argc, char ** argv) {
    mo_init(0);

    FILE *fp = fopen("music.wav", "r");
    fseek(fp, 0, SEEK_END);
    unsigned int sz = ftell(fp);
    fseek(fp, 0, SEEK_SET); 

    void *data = malloc(sz);
    fread(data, sz, 1, fp);

    fclose(fp);

    mo_audio_t *audio = mo_audio(data, sz, MO_AUDIO_STREAM);
    mo_play(audio);

    while (mo_is_playing(audio));

    // mocha_sound_stop(ctx, snd);

    mo_deinit();

    return 0;
}
