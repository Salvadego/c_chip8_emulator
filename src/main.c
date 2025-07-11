#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "../utils/types.h"
#include "SDL_error.h"
#include "SDL_log.h"
#include "SDL_video.h"

#define CHIP_WIDTH 64
#define CHIP_HEIGHT 32
#define SCALE 10

typedef struct {
        SDL_Window *window;
} sdl_t;

typedef struct {
        i32 window_width;
        i32 window_height;
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
            config.window_width,
            config.window_height,
            0);

        if (!sdl->window) {
                SDL_Log("Could not create SDL Window: %s", SDL_GetError());
                return false;
        }

        return true;
}

bool set_config_from_args(config_t *config, const int argc, char **argv) {
        *config = (config_t){
            .window_width = CHIP_WIDTH,
            .window_height = CHIP_HEIGHT,
        };

        for (int i = 1; i < argc; ++i) {
                (void)argv[i];
        }

        return true;
}

void fin_cleanup(sdl_t *sdl) {
        SDL_DestroyWindow(sdl->window);
        SDL_Quit();
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

        exit(EXIT_SUCCESS);
}
