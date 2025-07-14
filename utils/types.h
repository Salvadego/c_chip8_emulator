#include <SDL2/SDL.h>
#include <stdbool.h>
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

#define WHITE 0xFFFFFFFF
#define GREEN 0x55FF55FF
#define BLACK 0x000000FF

#define DISLPAY_SIZE 64 * 32
#define STACK_SIZE 12
#define REGISTERS_SIZE 16
#define KEYPAD_SIZE 16
#define Kilobytes(a) a * 1024

#define CLEAR_OPCODE 0x00E0
#define RETURN_OPCODE 0x00EE

typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int32_t u32;
typedef u_int64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// SDL container.
typedef struct {
        SDL_Window *window;
        SDL_Renderer *renderer;
} sdl_t;

// Configs for the emulator.
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
        DEBUG_LOG("Jump to address 0x%03X\n", chip8->inst.addr.NNN);
        chip8->PC = chip8->inst.addr.NNN;
}

void inst_2NNN(chip8_t *chip8) {
        DEBUG_LOG(
            "Call at 0x%03X | Push PC=0x%03X to stack\n",
            chip8->inst.addr.NNN,
            chip8->PC);

        *chip8->stack_ptr = chip8->PC;
        chip8->stack_ptr++;
        chip8->PC = chip8->inst.addr.NNN;
}

void inst_ANNN(chip8_t *chip8) {
	DEBUG_LOG("I = %d\n", chip8->inst.addr.NNN);
        chip8->I = chip8->inst.addr.NNN;
}

void inst_6XNN(chip8_t *chip8) {
	DEBUG_LOG("VX = %d\n", chip8->inst.reg_byte.KK);
        const u8 VxReg = chip8->inst.reg_byte.Vx;
        const u8 byte = chip8->inst.reg_byte.KK;
        chip8->V[VxReg] = byte;
}

void inst_7XNN(chip8_t *chip8) {
	DEBUG_LOG("VX += %d\n", chip8->inst.reg_byte.KK);
        const u8 VxReg = chip8->inst.reg_byte.Vx;
        const u8 byte = chip8->inst.reg_byte.KK;
        chip8->V[VxReg] += byte;
}

void inst_8XYN(chip8_t *chip8) {
        const u8 VxReg = chip8->inst.reg_reg.Vx;
	const u8 VyReg = chip8->inst.reg_reg.Vy;
        const u8 byte = chip8->inst.reg_reg.op;
	(void)VxReg;
	(void)VyReg;
	(void)byte;
        // chip8->V[VxReg] += byte;
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
                        SDL_Log(
                            "Unknown 0x0-family instruction: 0x%04X",
                            chip8->inst.opcode);
                        chip8->state = QUIT;
                        break;
        }
}

const instruction_handler_t instruction_table[INST_COUNT] = {
    [INST_0] = dispatch_zero_family,
    [INST_1] = inst_1NNN,
    [INST_2] = inst_2NNN,
    [INST_6] = inst_6XNN,
    [INST_7] = inst_7XNN,
    [INST_A] = inst_ANNN,
};
