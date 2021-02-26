#include "mocha.h"

int main(int argc, char ** argv) {
	Mocha *ctx = mocha_init(0);

	mo_Sound snd = mocha_sound_load(ctx, "music.wav", MO_AUDIO_STREAM);

	mocha_sound_play(ctx, snd);
	// mocha_sound_stop(ctx, snd);
	while (mocha_sound_playing(ctx, snd));

	mocha_terminate(ctx);

	return 0;
}