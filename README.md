# Mocha
a tiny c lib for handle audio

```c
#include "mocha.h"

int main(int argc, char ** argv) {

    mo_init(0);
    FILE* fp;
    fp = fopen("music.mp3", "rb");
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* data = malloc(len);
    fread(data, 1, len, fp);

    mo_audio_t *audio = mo_load_audio_from_memory(data, len, MO_AUDIO_STREAM);
    fclose(fp);

    while (mo_audio_is_any_playing(audio));

    free(data);
    mo_destroy_audio(audio);
    mo_quit();

    return 0;
}
```
