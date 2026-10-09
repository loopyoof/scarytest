// Wii U homebrew prototype: plays a JPEG-frame video + OGG audio, optional rumble
// and timed sound, then fakes a crash. Runs from RAM only. Power-cycle to recover.

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <coreinit/debug.h>
#include <vpad/input.h>
#include <stdio.h>
#include <string.h>

void OSFatal(const char *msg);

// ---- settings ----
#define MEDIA_DIR       "fs:/vol/external01/wiiu/scarytest"
#define FPS             15       // must match the ffmpeg conversion
#define DEBUG_NO_CRASH  0        // 1 = just exit after the video (for testing)
#define CRASH_MODE      1        // 0 = freeze last frame + buzz
                                 // 1 = show error.png + buzz
                                 // 2 = OSFatal system error screen
#define CRASH_MSG       "A fatal error has occurred."   // only used by mode 2
#define RUMBLE_START_MS 10000    // rumble starts this far into the video (0 = no rumble)
#define STING_MS        20000    // optional sound at this time (needs sting.ogg on the SD card)
#define STING_FILE      MEDIA_DIR "/sting.ogg"
#define ERROR_IMAGE     MEDIA_DIR "/error.png"
// ------------------

static void rumble_on(void)
{
    uint8_t pattern[120];
    memset(pattern, 0xFF, sizeof(pattern));
    VPADControlMotor(VPAD_CHAN_0, pattern, sizeof(pattern));
}

static void rumble_off(void) { VPADStopMotor(VPAD_CHAN_0); }

// Generates a harsh 1-second looping buzz (two square waves an octave apart).
static Mix_Chunk *make_buzz(void)
{
    int freq, channels;
    Uint16 format;
    if (!Mix_QuerySpec(&freq, &format, &channels)) return NULL;

    int period = freq / 200;                 // about 200 Hz
    int samples = period * 200;              // whole number of periods = clean loop
    int bytes = samples * channels * (int)sizeof(Sint16);
    Sint16 *buf = (Sint16 *)SDL_malloc(bytes);
    if (!buf) return NULL;

    for (int i = 0; i < samples; i++) {
        Sint16 a = ((i % period) < period / 2) ? 7000 : -7000;
        Sint16 b = ((i % (period / 2)) < period / 4) ? 7000 : -7000;
        for (int c = 0; c < channels; c++) buf[i * channels + c] = a + b;
    }
    Mix_Chunk *chunk = Mix_QuickLoad_RAW((Uint8 *)buf, bytes);
    if (chunk) Mix_VolumeChunk(chunk, MIX_MAX_VOLUME);
    return chunk;
}

static SDL_Texture *load_image(SDL_Renderer *r, const char *path)
{
    SDL_Surface *s = IMG_Load(path);
    if (!s) return NULL;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    return t;
}

static SDL_Texture *load_frame(SDL_Renderer *r, int idx)
{
    char path[256];
    snprintf(path, sizeof(path), MEDIA_DIR "/frames/f%05d.jpg", idx);
    return load_image(r, path);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    VPADInit();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return 1;
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    Mix_Init(MIX_INIT_OGG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    SDL_Window *win = SDL_CreateWindow("scarytest", 0, 0, 1280, 720, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    Mix_Music *music = Mix_LoadMUS(MEDIA_DIR "/audio.ogg");
    Mix_Chunk *sting = Mix_LoadWAV(STING_FILE);   // NULL if the file is missing, that's fine
    Mix_Chunk *buzz = make_buzz();

    SDL_Texture *tex = NULL;
    int cur = -1;
    int user_quit = 0;
    int sting_played = 0;
    Uint32 next_rumble = RUMBLE_START_MS;

    if (music) Mix_PlayMusic(music, 0);
    Uint32 start = SDL_GetTicks();

    while (!user_quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) user_quit = 1;   // HOME button exits cleanly
        }

        Uint32 elapsed = SDL_GetTicks() - start;

        if (RUMBLE_START_MS > 0 && elapsed >= next_rumble) {
            rumble_on();
            next_rumble = elapsed + 500;
        }

        if (sting && !sting_played && elapsed >= STING_MS) {
            Mix_VolumeChunk(sting, MIX_MAX_VOLUME);
            Mix_PlayChannel(-1, sting, 0);
            sting_played = 1;
        }

        int idx = 1 + (int)(elapsed * FPS / 1000);
        if (idx != cur) {
            SDL_Texture *next = load_frame(ren, idx);
            if (!next) break;                        // no more frames = video over
            if (tex) SDL_DestroyTexture(tex);
            tex = next;
            cur = idx;
        }

        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    if (!user_quit && !DEBUG_NO_CRASH) {
        Mix_HaltMusic();

#if CRASH_MODE == 2
        OSFatal(CRASH_MSG);
#else
        SDL_Texture *shown = tex;                    // mode 0: keep the last video frame
#if CRASH_MODE == 1
        SDL_Texture *err = load_image(ren, ERROR_IMAGE);
        if (err) shown = err;
#endif
        for (int i = 0; i < 3; i++) {                // draw a few times so both buffers match
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, shown, NULL, NULL);
            SDL_RenderPresent(ren);
        }
        if (buzz) Mix_PlayChannel(-1, buzz, -1);     // loop the buzz forever
        for (;;) SDL_Delay(1000);                    // frozen until power is held
#endif
    }

    if (tex) SDL_DestroyTexture(tex);
    rumble_off();
    Mix_HaltMusic();
    if (music) Mix_FreeMusic(music);
    if (sting) Mix_FreeChunk(sting);
    Mix_CloseAudio();
    IMG_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    VPADShutdown();
    return 0;
}
