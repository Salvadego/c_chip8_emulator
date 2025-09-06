#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef DEBUG
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif

#define CHIP_WIDTH 64
#define CHIP_HEIGHT 32
#define TIMER_DELAY_MS 16
#define SCALE_FACTOR 20
#define INSTRUCTIONS_PER_FRAME 10

#define DISLPAY_SIZE 64 * 32
#define STACK_SIZE 12
#define REGISTERS_SIZE 16
#define VF_REGISTER 0xF
#define KEYPAD_SIZE 16
#define SPRITE_WIDTH 8

#define FONT_CHAR_SIZE 5
#define FONT_START_ADDRESS 0

#define Byte(a) (a * 1)
#define Kilobytes(a) (a * Byte(1024))

#define CLEAR_OPCODE 0x00E0
#define RETURN_OPCODE 0x00EE

#define OPCODE_8XY0 0x0000
#define OPCODE_8XY1 0x0001
#define OPCODE_8XY2 0x0002
#define OPCODE_8XY3 0x0003
#define OPCODE_8XY4 0x0004
#define OPCODE_8XY5 0x0005
#define OPCODE_8XY6 0x0006
#define OPCODE_8XY7 0x0007
#define OPCODE_8XYE 0x000E

#define OPCODE_EX9E 0x009E
#define OPCODE_EXA1 0x00A1

#define OPCODE_FX07 0x0007
#define OPCODE_FX0A 0x000A
#define OPCODE_FX15 0x0015
#define OPCODE_FX18 0x0018
#define OPCODE_FX1E 0x001E
#define OPCODE_FX29 0x0029
#define OPCODE_FX33 0x0033
#define OPCODE_FX55 0x0055
#define OPCODE_FX65 0x0065

#define HUNDREDS 100
#define TENS 10

#define MSB_MASK 0x80
#define MSB_SHIFT 7

typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int32_t u32;
typedef u_int64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct {
        u8 red;
        u8 blue;
        u8 green;
        u8 alpha;
} color_t;

// Configs for the emulator.
typedef struct {
        i32 window_width;
        i32 window_height;
        i32 scale_factor;

        Color fg_color;  // RGBA8888
        Color bg_color;  // RGBA8888
} config_t;

// Emulator State
typedef enum {
        QUIT,
        RUNNING,
        PAUSED,
        NUM_OF_STATES,
} emulator_state_t;

const char *enum_state_lookup[NUM_OF_STATES] = {
    [QUIT] = "QUIT",
    [RUNNING] = "RUNNING",
    [PAUSED] = "PAUSED",
};

typedef union {
        u16 opcode;

        struct {
                u16 NNN : 12;
                u16 _ : 4;
        } addr;

        struct {
                u8 KK;
                u8 Vx : 4;
                u8 op : 4;
        } reg_byte;

        struct {
                u8 Vy : 4;
                u8 Vx : 4;
                u8 op : 4;
                u8 _ : 4;
        } reg_reg;

        struct {
                u8 nibble : 4;
                u8 Vy : 4;
                u8 Vx : 4;
                u8 op : 4;
        } reg_reg_nibble;

        struct {
                u8 key : 4;
                u8 Vx : 4;
                u8 op : 8;
        } key_op;

        struct {
                u8 Vx : 4;
                u16 op : 12;
        } timer_op;
} instruction_t;

typedef enum {
        INST_0,
        INST_1,
        INST_2,
        INST_3,
        INST_4,
        INST_5,
        INST_6,
        INST_7,
        INST_8,
        INST_9,
        INST_A,
        INST_B,
        INST_C,
        INST_D,
        INST_E,
        INST_F,
        INST_COUNT
} instruction_id_t;

typedef enum {
        INST_8XY0,
        INST_8XY1,
        INST_8XY2,
        INST_8XY3,
        INST_8XY4,
        INST_8XY5,
        INST_8XY6,
        INST_8XY7,
        INST_8XYE,
        INST_8_COUNT
} instruction_8_id_t;

typedef enum {
        INST_EX9E,
        INST_EXA1,
        INST_E_COUNT,
} instruction_E_id_t;

typedef enum {
        INST_FX07,
        INST_FX0A,
        INST_FX15,
        INST_FX18,
        INST_FX1E,
        INST_FX29,
        INST_FX33,
        INST_FX55,
        INST_FX65,
        INST_F_COUNT
} instruction_F_id_t;

// Chip8
typedef struct {
        emulator_state_t state;
        u8 ram[Kilobytes(4)];
        bool display[DISLPAY_SIZE];
        u16 stack[STACK_SIZE];
        u16 *stack_ptr;
        u8 V[REGISTERS_SIZE];  // Register V0 to VF
        u16 I;                 // Index register
        u16 PC;                // Program Counter
        u8 delay_timer;        // -- at 60hz when > 0
        u8 sound_timer;        // -- at 60hz when > 0 and plays tone when > 0
        bool keypad[KEYPAD_SIZE];  // Hex keypad 0-F
        const char *rom_name;      // Name of the file emulating
        instruction_t inst;        // current instruction
} chip8_t;

typedef void (*instruction_handler_t)(chip8_t *);

void inst_0NNN(chip8_t *chip8) {
        (void)chip8;  // Do nothing
}

void inst_00E0(chip8_t *chip8) {
        DEBUG_LOG("Clear screen\n");
        memset(chip8->display, 0, sizeof(chip8->display));
}

void inst_00EE(chip8_t *chip8) {
        chip8->stack_ptr--;
        chip8->PC = *chip8->stack_ptr;
}

void inst_1NNN(chip8_t *chip8) {
        DEBUG_LOG("Jump to address 0x%04X\n", chip8->inst.addr.NNN);
        chip8->PC = chip8->inst.addr.NNN;
}

void inst_2NNN(chip8_t *chip8) {
        DEBUG_LOG(
            "Call at 0x%04X | Push PC=0x%04X to stack\n",
            chip8->inst.addr.NNN,
            chip8->PC);

        *chip8->stack_ptr = chip8->PC;
        chip8->stack_ptr++;
        chip8->PC = chip8->inst.addr.NNN;
}

void inst_3XNN(chip8_t *chip8) {
        const u8 VxReg = chip8->inst.reg_byte.Vx;
        const u8 byte = chip8->inst.reg_byte.KK;
        DEBUG_LOG("If V%X == 0x%02X, skip next instruction\n", VxReg, byte);
        if (chip8->V[VxReg] == byte) {
                chip8->PC += 2;
        }
}

void inst_ANNN(chip8_t *chip8) {
        DEBUG_LOG("I = 0x%04X\n", chip8->inst.addr.NNN);
        chip8->I = chip8->inst.addr.NNN;
}

void inst_6XNN(chip8_t *chip8) {
        const u8 VxReg = chip8->inst.reg_byte.Vx;
        const u8 byte = chip8->inst.reg_byte.KK;
        DEBUG_LOG("V%X = 0x%02X\n", VxReg, byte);
        chip8->V[VxReg] = byte;
}

void inst_7XNN(chip8_t *chip8) {
        const u8 VxReg = chip8->inst.reg_byte.Vx;
        const u8 byte = chip8->inst.reg_byte.KK;
        DEBUG_LOG("V%X += 0x%02X\n", VxReg, byte);
        chip8->V[VxReg] += byte;
}

void inst_DXYN(chip8_t *chip8) {
        const u8 X_CORD = chip8->inst.reg_reg_nibble.Vx;
        const u8 Y_CORD = chip8->inst.reg_reg_nibble.Vy;
        const u8 nibble = chip8->inst.reg_reg_nibble.nibble;

        const u8 dxc = chip8->V[X_CORD] % CHIP_WIDTH;
        const u8 dyc = chip8->V[Y_CORD] % CHIP_HEIGHT;

        chip8->V[VF_REGISTER] = 0;

        for (u8 row = 0; row < nibble; row++) {
                const u8 sprite = chip8->ram[chip8->I + row];

                for (u8 col = 0; col < SPRITE_WIDTH; col++) {
                        const u8 sprite_pixel = (sprite >> (7 - col)) & 0x1;
                        if ((dyc + row) >= CHIP_HEIGHT ||
                            (dxc + col) >= CHIP_WIDTH) {
                                break;
                        }
                        const u16 display_index =
                            ((dyc + row) * CHIP_WIDTH) + (dxc + col);

                        if (sprite_pixel) {
                                if (chip8->display[display_index]) {
                                        chip8->V[VF_REGISTER] = 1;
                                }
                                chip8->display[display_index] ^= 1;
                        }
                }
        }

        DEBUG_LOG(
            "Draw sprite at Vx: %X, Vy: %X, height: %X\n",
            X_CORD,
            Y_CORD,
            nibble);
}

void dispatch_zero_family(chip8_t *chip8) {
        switch (chip8->inst.opcode) {
                case CLEAR_OPCODE:
                        inst_00E0(chip8);
                        break;
                case RETURN_OPCODE:
                        inst_00EE(chip8);
                        break;
                default:
                        DEBUG_LOG(
                            "Unknown 0x0-family instruction: 0x%04X",
                            chip8->inst.opcode);
                        chip8->state = QUIT;
                        break;
        }
}

void inst_4XNN(chip8_t *chip8) {
        const u8 Vx = chip8->inst.reg_byte.Vx;
        const u8 byte = chip8->inst.reg_byte.KK;
        if (chip8->V[Vx] != byte) {
                chip8->PC += 2;
        }
}

void inst_5XY0(chip8_t *chip8) {
        const u8 Vx = chip8->inst.reg_reg.Vx;
        const u8 Vy = chip8->inst.reg_reg.Vy;
        if (chip8->V[Vx] == chip8->V[Vy]) {
                chip8->PC += 2;
        }
}

void inst_8XY0(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        c->V[X] = c->V[Y];
}

void inst_8XY1(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        c->V[X] |= c->V[Y];
}

void inst_8XY2(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        c->V[X] &= c->V[Y];
}

void inst_8XY3(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        c->V[X] ^= c->V[Y];
}

void inst_8XY4(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        u16 sum = c->V[X] + c->V[Y];
        c->V[VF_REGISTER] = sum > 0xFF;  // Carry flag
        c->V[X] = (u8)(sum & 0xFF);
}

void inst_8XY5(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        c->V[VF_REGISTER] = (c->V[X] >= c->V[Y]);  // borrow flag
        c->V[X] -= c->V[Y];
}

void inst_8XY6(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        // Algumas versões usam Vy em vez de Vx, mas a mais comum é Vx.
        c->V[VF_REGISTER] = c->V[X] & 0x1;  // LSB antes de shift
        c->V[X] >>= 1;
}

void inst_8XY7(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        c->V[VF_REGISTER] = (c->V[Y] >= c->V[X]);  // borrow flag
        c->V[X] = c->V[Y] - c->V[X];
}

void inst_8XYE(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        c->V[VF_REGISTER] = (c->V[X] & 0x80) >> 7;  // MSB antes do shift
        c->V[X] <<= 1;
}

void inst_9XY0(chip8_t *c) {
        u8 X = (c->inst.opcode >> 8) & 0xF;
        u8 Y = (c->inst.opcode >> 4) & 0xF;
        if (c->V[X] != c->V[Y]) {
                c->PC += 2;
        }
}

void inst_BNNN(chip8_t *chip8) {
        chip8->PC = chip8->inst.addr.NNN + chip8->V[0];
}

void inst_CXNN(chip8_t *chip8) {
        const u8 Vx = chip8->inst.reg_byte.Vx;
        const u8 KK = chip8->inst.reg_byte.KK;
        chip8->V[Vx] = (rand() & REGISTERS_SIZE) & KK;
}

void inst_EX9E(chip8_t *chip8) {
        const u8 Vx = chip8->inst.reg_byte.Vx;
        if (chip8->keypad[chip8->V[Vx]]) {
                chip8->PC += 2;
        }
}

void inst_EXA1(chip8_t *chip8) {
        const u8 Vx = chip8->inst.reg_byte.Vx;
        if (!chip8->keypad[chip8->V[Vx]]) {
                chip8->PC += 2;
        }
}

void inst_FX07(chip8_t *chip8) {
        chip8->V[chip8->inst.reg_byte.Vx] = chip8->delay_timer;
}

void inst_FX0A(chip8_t *chip8) {
        for (u8 i = 0; i < KEYPAD_SIZE; ++i) {
                if (chip8->keypad[i]) {
                        chip8->V[chip8->inst.reg_byte.Vx] = i;
                        return;
                }
        }
        chip8->PC -= 2;
}

void inst_FX15(chip8_t *chip8) {
        chip8->delay_timer = chip8->V[chip8->inst.reg_byte.Vx];
}

void inst_FX18(chip8_t *chip8) {
        chip8->sound_timer = chip8->V[chip8->inst.reg_byte.Vx];
}

void inst_FX1E(chip8_t *chip8) {
        chip8->I += chip8->V[chip8->inst.reg_byte.Vx];
}

void inst_FX29(chip8_t *chip8) {
        chip8->I = 0 + (chip8->V[chip8->inst.reg_byte.Vx] * FONT_CHAR_SIZE);
}

void inst_FX33(chip8_t *chip8) {
        const u8 val = chip8->V[chip8->inst.reg_byte.Vx];
        chip8->ram[chip8->I] = val / HUNDREDS;
        chip8->ram[chip8->I + 1] = (val / TENS) % TENS;
        chip8->ram[chip8->I + 2] = val % TENS;
}

void inst_FX55(chip8_t *chip8) {
        const u8 Vx = chip8->inst.reg_byte.Vx;
        for (u8 i = 0; i <= Vx; ++i) {
                chip8->ram[chip8->I + i] = chip8->V[i];
        }
}

void inst_FX65(chip8_t *chip8) {
        const u8 Vx = chip8->inst.reg_byte.Vx;
        for (u8 i = 0; i <= Vx; ++i) {
                chip8->V[i] = chip8->ram[chip8->I + i];
        }
}

void dispatch_eight_family(chip8_t *chip8) {
        switch (chip8->inst.opcode & 0x000F) {  // NOLINT
                case OPCODE_8XY0:
                        inst_8XY0(chip8);
                        break;
                case OPCODE_8XY1:
                        inst_8XY1(chip8);
                        break;
                case OPCODE_8XY2:
                        inst_8XY2(chip8);
                        break;
                case OPCODE_8XY3:
                        inst_8XY3(chip8);
                        break;
                case OPCODE_8XY4:
                        inst_8XY4(chip8);
                        break;
                case OPCODE_8XY5:
                        inst_8XY5(chip8);
                        break;
                case OPCODE_8XY6:
                        inst_8XY6(chip8);
                        break;
                case OPCODE_8XY7:
                        inst_8XY7(chip8);
                        break;
                case OPCODE_8XYE:
                        inst_8XYE(chip8);
                        break;
                default:
                        DEBUG_LOG(
                            "Unknown 0x8-family instruction: 0x%04X",
                            chip8->inst.opcode);
                        chip8->state = QUIT;
                        break;
        }
}

void dispatch_E_family(chip8_t *chip8) {
        switch (chip8->inst.opcode & 0x00FF) {  // NOLINT
                case OPCODE_EX9E:
                        inst_EX9E(chip8);
                        break;
                case OPCODE_EXA1:
                        inst_EXA1(chip8);
                        break;
                default:
                        DEBUG_LOG(
                            "Unknown 0xE-family instruction: 0x%04X",
                            chip8->inst.opcode);
                        chip8->state = QUIT;
                        break;
        }
}

void dispatch_F_family(chip8_t *chip8) {
        switch (chip8->inst.opcode & 0x00FF) {  // NOLINT
                case OPCODE_FX07:
                        inst_FX07(chip8);
                        break;
                case OPCODE_FX0A:
                        inst_FX0A(chip8);
                        break;
                case OPCODE_FX15:
                        inst_FX15(chip8);
                        break;
                case OPCODE_FX18:
                        inst_FX18(chip8);
                        break;
                case OPCODE_FX1E:
                        inst_FX1E(chip8);
                        break;
                case OPCODE_FX29:
                        inst_FX29(chip8);
                        break;
                case OPCODE_FX33:
                        inst_FX33(chip8);
                        break;
                case OPCODE_FX55:
                        inst_FX55(chip8);
                        break;
                case OPCODE_FX65:
                        inst_FX65(chip8);
                        break;
                default:
                        DEBUG_LOG(
                            "Unknown 0xF-family instruction: 0x%04X",
                            chip8->inst.opcode);
                        chip8->state = QUIT;
                        break;
        }
}

static const instruction_handler_t instruction_table[INST_COUNT] = {
    [INST_0] = dispatch_zero_family,
    [INST_1] = inst_1NNN,
    [INST_2] = inst_2NNN,
    [INST_3] = inst_3XNN,
    [INST_4] = inst_4XNN,
    [INST_5] = inst_5XY0,
    [INST_6] = inst_6XNN,
    [INST_7] = inst_7XNN,
    [INST_8] = dispatch_eight_family,
    [INST_9] = inst_9XY0,
    [INST_A] = inst_ANNN,
    [INST_B] = inst_BNNN,
    [INST_C] = inst_CXNN,
    [INST_D] = inst_DXYN,
    [INST_E] = dispatch_E_family,
    [INST_F] = dispatch_F_family,
};
