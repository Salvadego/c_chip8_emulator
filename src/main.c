#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../utils/types.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_render.h"
#include "SDL_timer.h"
#include "SDL_video.h"

#define CHIP_WIDTH 64
#define CHIP_HEIGHT 32
#define TIMER_DELAY_MS 16
#define SCALE_FACTOR 20

#define WHITE 0xFFFFFFFF
#define GREEN 0x55FF55FF

// SDL container.
typedef struct {
        SDL_Window *window;
        SDL_Renderer *renderer;
} sdl_t;

// Configs for the tmulator.
typedef struct {
        i32 window_width;
        i32 window_height;
        i32 scale_factor;

        u32 fg_color;  // RGBA8888
        u32 bg_color;  // RGBA8888
} config_t;

// Emulator State
typedef enum {
        QUIT,
        RUNNING,
        PAUSED,
} emulator_state_t;

typedef struct {
        emulator_state_t state;
} chip8_t;

typedef enum {
        EVENT_OK,
        EVENT_ERROR,
} event_status_return_t;

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

bool init_chip8(chip8_t *chip8) {
        chip8->state = RUNNING;
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

event_status_return_t handle_events(SDL_Event event, chip8_t *chip8) {
        switch (event.type) {
                case SDL_QUIT:
                        chip8->state = QUIT;
                        return EVENT_OK;
                case SDL_KEYDOWN:
                        printf(
                            "Keydown %d: | State: %d | Mods: %d\n",
                            event.key.keysym.sym,
                            event.key.state,
                            event.key.keysym.mod);
                        switch (event.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                        chip8->state = QUIT;
                                        return EVENT_OK;
                                default:
                                        return EVENT_OK;
                        }
                case SDL_KEYUP:
                default:
                        return EVENT_OK;
        }
}

void handle_input(chip8_t *chip8) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
                handle_events(event, chip8);
		break;
        }
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

        chip8_t chip8 = {0};
        if (!init_chip8(&chip8)) {
                exit(EXIT_FAILURE);
        }

        clear_screen(conf, sdl);

        while (chip8.state != QUIT) {
                handle_input(&chip8);

                // time()
                //
                // TODO: emulation comoes  here
                //
                // time() elapsed since time()

                SDL_Delay(TIMER_DELAY_MS);  // TIMER_DELAY_MS - dt (delta time)
                update_screen(sdl);
        }

        fin_cleanup(sdl);
        exit(EXIT_SUCCESS);
}
