#include "mocha.h"

int main(int argc, char ** argv) {
    mocha_init(0);

    mo_Sound snd = mocha_sound_load("music.wav", MO_AUDIO_STREAM);

    mocha_sound_play(snd);
    // mocha_sound_stop(ctx, snd);
    while (mocha_sound_playing(snd));

    mocha_terminate();

    return 0;
}
