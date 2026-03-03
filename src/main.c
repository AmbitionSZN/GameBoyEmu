#include "emu.h"
#include "ppu.h"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include "cart.h"
#include "cpu.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdint.h>
#include <stdio.h>

Emulator emu;
CPU cpu = {0};
uint8_t memory[0x10000] = {0};
Cartridge cart;
FILE *logFile;

extern Ppu ppu;

static SDL_Window *window = NULL;
static SDL_Window *tileWindow = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Renderer *tileRenderer = NULL;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetAppMetadata("Example Renderer Clear", "1.0",
                       "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", 640, 480, 0,
                                     &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("Tile Window", 640, 480, 0, &tileWindow,
                                     &tileRenderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 640, 480,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderLogicalPresentation(tileRenderer, 640, 480,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    logFile = fopen("../logs/log.txt", "w");

    cart = LoadCartridge("../roms/Dr.Mario.gb");
    opcodesJsonParser("../Opcodes.json");
    cpuInit();

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS.
                                 */
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

	static uint32_t prevFrame;

    while  (prevFrame == ppu.CurrentFrame) {
        cpuStep();
    }
    SDL_RenderClear(tileRenderer);

    const double now = ((double)SDL_GetTicks()) / 1000.0;
    const float red = (float)(0.5 + 0.5 * SDL_sin(now));
    const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));

    SDL_SetRenderDrawColorFloat(renderer, red, green, blue,
                                SDL_ALPHA_OPAQUE_FLOAT);
    int w, h;
    SDL_GetWindowSize(tileWindow, &w, &h);

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    renderTiles(tileRenderer, w, h);
    SDL_RenderPresent(tileRenderer);

	prevFrame = ppu.CurrentFrame;

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    fclose(logFile);
    /* SDL will clean up the window/renderer for us. */
}
