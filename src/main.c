// Wii U homebrew prototype: plays a JPEG-frame video + OGG audio, then "crashes".
// Runs from RAM only. Writes nothing to the console. Power-cycle to recover.

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
   #include <coreinit/debug.h>
   void OSFatal(const char *msg);
#include <stdio.h>

// ---- settings ----
#define MEDIA_DIR       "fs:/vol/external01/wiiu/scarytest"  // SD card folder
#define FPS             15       // must match make_assets.py
#define DEBUG_NO_CRASH  0        // 1 = just exit to menu after the video (use while testing!)
#define CRASH_MODE      0        // 0 = OSFatal error screen, 1 = black-screen freeze
#define CRASH_PAUSE_MS  1500     // black screen before the crash
#define CRASH_MSG       "A fatal error has occurred."
// ------------------

static SDL_Texture *load_frame(SDL_Renderer *r, int idx)
{
    char path[256];
    snprintf(path, sizeof(path), MEDIA_DIR "/frames/f%05d.jpg", idx);
    SDL_Surface *s = IMG_Load(path);
    if (!s) return NULL;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    return t;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return 1;
    IMG_Init(IMG_INIT_JPG);
    Mix_Init(MIX_INIT_OGG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    SDL_Window *win = SDL_CreateWindow("scarytest", 0, 0, 1280, 720, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    Mix_Music *music = Mix_LoadMUS(MEDIA_DIR "/audio.ogg");

    SDL_Texture *tex = NULL;
    int cur = -1;
    int user_quit = 0;

    if (music) Mix_PlayMusic(music, 0);
    Uint32 start = SDL_GetTicks();

    while (!user_quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) user_quit = 1;   // HOME button exits cleanly
        }

        int idx = 1 + (int)((SDL_GetTicks() - start) * FPS / 1000);
        if (idx != cur) {
            SDL_Texture *next = load_frame(ren, idx);
            if (!next) break;                        // no more frames = video over
            if (tex) SDL_DestroyTexture(tex);
            tex = next;
            cur = idx;
        }

        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);        // stretch to full screen
        SDL_RenderPresent(ren);
    }

    if (tex) SDL_DestroyTexture(tex);
    Mix_HaltMusic();

    if (!user_quit && !DEBUG_NO_CRASH) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderPresent(ren);
        SDL_Delay(CRASH_PAUSE_MS);

#if CRASH_MODE == 0
        OSFatal(CRASH_MSG);
#else
        for (;;) SDL_Delay(1000);                    // frozen black screen
#endif
    }

    if (music) Mix_FreeMusic(music);
    Mix_CloseAudio();
    IMG_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
