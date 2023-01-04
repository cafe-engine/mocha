/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/05/2021 19:12:13
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "mocha.h"

int main(int argc, char ** argv) {
    mo_init(0);

    FILE* fp;
    fp = fopen("examples/hello/music.wav", "rb");
    int len;
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* data = malloc(len);
    fread(data, 1, len, fp);
    mo_audio_t* audio = mo_load_audio_from_memory(data, len, MO_AUDIO_STREAM);
    mo_play_audio(audio);

    printf("press 'p' to stop...\n");
    while (mo_audio_is_any_playing(audio)) {
        int c = getchar();
        if (c == 'p') break;
    }

    free(data);
    fclose(fp);
    mo_destroy_audio(audio);
    mo_quit();

    return 0;
}
