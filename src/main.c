#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../utils/types.h"

#define TARGET_FPS 60
#define SECOND 1000.0f

bool init_raylib(config_t config) {
        InitWindow(
            config.window_width * config.scale_factor,
            config.window_height * config.scale_factor,
            "Chip8 Emulator");
        SetTargetFPS(TARGET_FPS);
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
                TraceLog(
                    LOG_ERROR,
                    "Rom file %s is invalid or does not exist...",
                    rom_name);
                return false;
        }
        if (fseek(rom, 0, SEEK_END) != 0) {
                TraceLog(LOG_ERROR, "Couldn't move cursor to end of file\n");
                return false;
        };

        const size_t rom_size = ftell(rom);
        const size_t max_size = sizeof(chip8->ram) - entry_point;
        if (fseek(rom, 0, SEEK_SET) != 0) {
                TraceLog(
                    LOG_ERROR, "Couldn't move cursor to beginning of file\n");
                return false;
        }

        if (rom_size > max_size) {
                TraceLog(
                    LOG_ERROR,
                    "Rom file %s is too big for this chip8, max size: %zu",
                    rom_name,
                    max_size);
                return false;
        }

        if (fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1) {
                TraceLog(
                    LOG_ERROR, "Couldn't read rom: %s, into ram\n", rom_name);
                return false;
        }
        fclose(rom);

        chip8->state = RUNNING;
        chip8->stack_ptr = &chip8->stack[0];
        chip8->PC = entry_point;
        chip8->rom_name = rom_name;

        return true;
}

void fin_cleanup() {
        CloseWindow();
}

void clear_screen(const config_t config) {
        ClearBackground(*(Color *)&config.bg_color);
}

void update_screen(const config_t config, const chip8_t chip8) {
        BeginDrawing();
        clear_screen(config);

        Color fg_ray_color = *(Color *)&config.fg_color;

        for (u32 row = 0; row < sizeof chip8.display; row++) {
                if (chip8.display[row]) {
                        DrawRectangle(
                            (int)(row % config.window_width) *
                                config.scale_factor,
                            (int)(row / config.window_width) *
                                config.scale_factor,
                            config.scale_factor,
                            config.scale_factor,
                            fg_ray_color);
                }
        }
        EndDrawing();
}

void handle_input_raylib(chip8_t *chip8) {
        if (WindowShouldClose()) {
                chip8->state = QUIT;
                return;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
                chip8->state = QUIT;
                return;
        }
        if (IsKeyPressed(KEY_SPACE)) {
                chip8->state = chip8->state == RUNNING ? PAUSED : RUNNING;
                return;
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
                TraceLog(
                    LOG_ERROR,
                    "Unimplemented instruction: 0x%04X",
                    chip8->inst.opcode);

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

        if (!init_raylib(conf)) {
                exit(EXIT_FAILURE);
        }

        chip8_t chip8 = {0};
        const char *rom_name = argv[1];
        if (!init_chip8(&chip8, rom_name)) {
                fin_cleanup();
                exit(EXIT_FAILURE);
        }

        clear_screen(conf);

        emulator_state_t curr_state = chip8.state;
        while (chip8.state != QUIT) {
                handle_input_raylib(&chip8);
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

                WaitTime(TIMER_DELAY_MS / SECOND);

                update_screen(conf, chip8);
        }

        fin_cleanup();
        exit(EXIT_SUCCESS);
}
