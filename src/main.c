#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "../utils/types.h"
#include "SDL_error.h"
#include "SDL_log.h"
#include "SDL_render.h"
#include "SDL_timer.h"
#include "SDL_video.h"

#define CHIP_WIDTH 64
#define CHIP_HEIGHT 32
#define TIMER_DELAY_MS 16
#define SCALE_FACTOR 10

#define WHITE 0xFFFFFFFF
#define GREEN 0x55FF55FF

typedef struct {
        SDL_Window *window;
        SDL_Renderer *renderer;
} sdl_t;

typedef struct {
        i32 window_width;
        i32 window_height;
        i32 scale_factor;

        u32 fg_color;  // RGBA8888
        u32 bg_color;  // RGBA8888
} config_t;

bool init_sdl(sdl_t *sdl, const config_t config) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
                SDL_Log(
                    "Could not initalize SDL subsystems! %s\n", SDL_GetError());
                return false;
        };

        sdl->window = SDL_CreateWindow(
            "Chip8 Emulator",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            config.window_width * config.scale_factor,
            config.window_height * config.scale_factor,
            0);

        if (!sdl->window) {
                SDL_Log("Could not create SDL Window: %s", SDL_GetError());
                return false;
        }

        sdl->renderer =
            SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);

        if (!sdl->renderer) {
                SDL_Log("Could not create SDL Renderer: %s", SDL_GetError());
                return false;
        }

        return true;
}

bool set_config_from_args(config_t *config, const int argc, char **argv) {
        *config = (config_t){
            .window_width = CHIP_WIDTH,
            .window_height = CHIP_HEIGHT,
            .scale_factor = SCALE_FACTOR,

            .fg_color = GREEN,
            .bg_color = GREEN,
        };

        for (int i = 1; i < argc; ++i) {
                (void)argv[i];
        }

        return true;
}

void fin_cleanup(const sdl_t sdl) {
        SDL_DestroyRenderer(sdl.renderer);
        SDL_DestroyWindow(sdl.window);
        SDL_Quit();
}

void clear_screen(const config_t config, const sdl_t sdl) {
        const u8 red = (config.bg_color >> 24) & 0xFF;
        const u8 green = (config.bg_color >> 16) & 0xFF;
        const u8 blue = (config.bg_color >> 8) & 0xFF;
        const u8 alpha = (config.bg_color >> 0) & 0xFF;

        SDL_SetRenderDrawColor(sdl.renderer, red, green, blue, alpha);
        SDL_RenderClear(sdl.renderer);
}

void update_screen(const sdl_t sdl) {
        SDL_RenderPresent(sdl.renderer);
}

int main(int argc, char *argv[]) {
        config_t conf = {0};
        if (!set_config_from_args(&conf, argc, argv)) {
                exit(EXIT_FAILURE);
        }

        sdl_t sdl = {0};
        if (!init_sdl(&sdl, conf)) {
                exit(EXIT_FAILURE);
        }

        clear_screen(conf, sdl);

        while (true) {
                // time()
                //
                // TODO: emulation comoes  here
                //
                // time() elapsed since time()
                SDL_Delay(TIMER_DELAY_MS);
                update_screen(sdl);
        }

        fin_cleanup(sdl);
        exit(EXIT_SUCCESS);
}
