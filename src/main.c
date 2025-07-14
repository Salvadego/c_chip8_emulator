#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../utils/types.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_render.h"
#include "SDL_timer.h"
#include "SDL_video.h"

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
            .bg_color = BLACK,
        };

        for (int i = 1; i < argc; ++i) {
                (void)argv[i];
        }

        return true;
}

bool init_chip8(chip8_t *chip8, const char rom_name[]) {
        const u32 entry_point = 0x200;
        const u8 font[] = {
            0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
            0x20, 0x60, 0x20, 0x20, 0x70,  // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
            0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
            0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
            0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
            0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
            0xF0, 0x80, 0xF0, 0x80, 0x80   // F
        };

        memcpy(&chip8->ram[0], font, sizeof(font));

        FILE *rom = fopen(rom_name, "rb");
        if (!rom) {
                SDL_Log(
                    "Rom file %s is invalid or does not exist...\n", rom_name);
                return false;
        }
        if (fseek(rom, 0, SEEK_END) != 0) {
                SDL_Log("Couldn't move cursor to end of file\n");
                return false;
        };

        const size_t rom_size = ftell(rom);
        const size_t max_size = sizeof(chip8->ram) - entry_point;
        if (fseek(rom, 0, SEEK_SET) != 0) {
                SDL_Log("Couldn't move cursor to beggining of file\n");
                return false;
        }

        if (rom_size > max_size) {
                SDL_Log(
                    "Rom file %s is too big for this chip8, max size: %zu",
                    rom_name,
                    max_size);
                return false;
        }

        if (fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1) {
                SDL_Log("Couldn't read rom: %s, into ram\n", rom_name);
                return false;
        }
        fclose(rom);

        // defaults
        chip8->state = RUNNING;
        chip8->stack_ptr = &chip8->stack[0];
        chip8->PC = entry_point;
        chip8->rom_name = rom_name;

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

void handle_events(SDL_Event event, chip8_t *chip8) {
        switch (event.type) {
                case SDL_QUIT:
                        chip8->state = QUIT;
                        return;
                case SDL_KEYDOWN:
                        DEBUG_LOG(
                            "Keydown %d: | State: %d | Mods: %d\n",
                            event.key.keysym.sym,
                            event.key.state,
                            event.key.keysym.mod);
                        switch (event.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                        chip8->state = QUIT;
                                        return;
                                case SDLK_SPACE:
                                        chip8->state = chip8->state == RUNNING
                                                           ? PAUSED
                                                           : RUNNING;
                                        return;
                                default:
                                        return;
                        }
                case SDL_KEYUP:
                default:
                        return;
        }
}

void handle_input(chip8_t *chip8) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
                handle_events(event, chip8);
                break;
        }
}

void emulate_instruction(chip8_t *chip8) {
        const u8 byte = 8;
        const u8 mask = 0xF;
        const u8 high = chip8->ram[chip8->PC];
        const u8 low = chip8->ram[chip8->PC + 1];
        chip8->inst.opcode = (high << byte) | low;
        chip8->PC += 2;

        DEBUG_LOG(
            "PC: 0x%03X | Opcode: 0x%04X | ", chip8->PC, chip8->inst.opcode);

        u8 op_high_nibble = (chip8->inst.opcode >> (byte + 4)) & mask;
        instruction_handler_t handler = instruction_table[op_high_nibble];
        if (!handler) {
		DEBUG_LOG("\n");
                SDL_Log(
                    "Unimplemented instruction: 0x%04X", chip8->inst.opcode);

#ifndef DEBUG
                chip8->state = QUIT;
#endif

                return;
        }
        handler(chip8);
}

int main(int argc, char *argv[]) {
        if (argc < 2) {
                fprintf(stderr, "Usage: %s <rom_file>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        config_t conf = {0};
        if (!set_config_from_args(&conf, argc, argv)) {
                exit(EXIT_FAILURE);
        }

        sdl_t sdl = {0};
        if (!init_sdl(&sdl, conf)) {
                exit(EXIT_FAILURE);
        }

        chip8_t chip8 = {0};
        const char *rom_name = argv[1];
        if (!init_chip8(&chip8, rom_name)) {
                exit(EXIT_FAILURE);
        }

        clear_screen(conf, sdl);

        emulator_state_t curr_state = chip8.state;
        while (chip8.state != QUIT) {
                handle_input(&chip8);
                if (chip8.state != curr_state) {
                        curr_state = chip8.state;
                        DEBUG_LOG(
                            "Changed State: %s\n",
                            enum_state_lookup[curr_state]);
                }

                if (chip8.state == PAUSED) {
                        continue;
                }

                emulate_instruction(&chip8);

                SDL_Delay(TIMER_DELAY_MS);  // TIMER_DELAY_MS - dt (delta time)
                update_screen(sdl);
        }

        fin_cleanup(sdl);
        exit(EXIT_SUCCESS);
}
