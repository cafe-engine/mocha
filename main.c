#include "mocha.h"

int main(int argc, char ** argv) {
    mo_init(0);

    FILE *fp = fopen("music.wav", "r");
    fseek(fp, 0, SEEK_END);
    unsigned int sz = ftell(fp);
    fseek(fp, 0, SEEK_SET); 

    char data[sz];
    fread(data, sz, 1, fp);

    fclose(fp);

    mo_audio_t *audio = mo_audio(data, sz, MO_AUDIO_STATIC);
    mo_play(audio);

    char run = 1;

    // while (run) scanf("%c", &run);
    while (mo_is_playing(audio));

    mo_audio_destroy(audio);

    // mocha_sound_stop(ctx, snd);

    mo_deinit();

    return 0;
}
