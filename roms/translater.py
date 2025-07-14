import sys

def decode_instruction(opcode):
    nnn = opcode & 0x0FFF
    n   = opcode & 0x000F
    x   = (opcode & 0x0F00) >> 8
    y   = (opcode & 0x00F0) >> 4
    kk  = opcode & 0x00FF

    if opcode == 0x00E0:
        return "disp_clear()"
    elif opcode == 0x00EE:
        return "return"
    elif opcode & 0xF000 == 0x0000:
        return f"call_machine(0x{nnn:03X})"
    elif opcode & 0xF000 == 0x1000:
        return f"goto 0x{nnn:03X}"
    elif opcode & 0xF000 == 0x2000:
        return f"*(0x{nnn:03X})()"
    elif opcode & 0xF000 == 0x3000:
        return f"if (V[{x:X}] == 0x{kk:02X}) skip"
    elif opcode & 0xF000 == 0x4000:
        return f"if (V[{x:X}] != 0x{kk:02X}) skip"
    elif opcode & 0xF00F == 0x5000:
        return f"if (V[{x:X}] == V[{y:X}]) skip"
    elif opcode & 0xF000 == 0x6000:
        return f"V[{x:X}] = 0x{kk:02X}"
    elif opcode & 0xF000 == 0x7000:
        return f"V[{x:X}] += 0x{kk:02X}"
    elif opcode & 0xF00F == 0x8000:
        return f"V[{x:X}] = V[{y:X}]"
    elif opcode & 0xF00F == 0x8001:
        return f"V[{x:X}] |= V[{y:X}]"
    elif opcode & 0xF00F == 0x8002:
        return f"V[{x:X}] &= V[{y:X}]"
    elif opcode & 0xF00F == 0x8003:
        return f"V[{x:X}] ^= V[{y:X}]"
    elif opcode & 0xF00F == 0x8004:
        return f"V[{x:X}] += V[{y:X}]; VF = carry"
    elif opcode & 0xF00F == 0x8005:
        return f"V[{x:X}] -= V[{y:X}]; VF = !borrow"
    elif opcode & 0xF00F == 0x8006:
        return f"V[{x:X}] >>= 1; VF = LSB(V[{x:X}])"
    elif opcode & 0xF00F == 0x8007:
        return f"V[{x:X}] = V[{y:X}] - V[{x:X}]; VF = !borrow"
    elif opcode & 0xF00F == 0x800E:
        return f"V[{x:X}] <<= 1; VF = MSB(V[{x:X}])"
    elif opcode & 0xF00F == 0x9000:
        return f"if (V[{x:X}] != V[{y:X}]) skip"
    elif opcode & 0xF000 == 0xA000:
        return f"I = 0x{nnn:03X}"
    elif opcode & 0xF000 == 0xB000:
        return f"PC = V[0] + 0x{nnn:03X}"
    elif opcode & 0xF000 == 0xC000:
        return f"V[{x:X}] = rand() & 0x{kk:02X}"
    elif opcode & 0xF000 == 0xD000:
        return f"draw(V[{x:X}], V[{y:X}], 0x{n:X})"
    elif opcode & 0xF0FF == 0xE09E:
        return f"if (key == V[{x:X}]) skip"
    elif opcode & 0xF0FF == 0xE0A1:
        return f"if (key != V[{x:X}]) skip"
    elif opcode & 0xF0FF == 0xF007:
        return f"V[{x:X}] = get_delay()"
    elif opcode & 0xF0FF == 0xF00A:
        return f"V[{x:X}] = wait_key()"
    elif opcode & 0xF0FF == 0xF015:
        return f"set_delay(V[{x:X}])"
    elif opcode & 0xF0FF == 0xF018:
        return f"set_sound(V[{x:X}])"
    elif opcode & 0xF0FF == 0xF01E:
        return f"I += V[{x:X}]"
    elif opcode & 0xF0FF == 0xF029:
        return f"I = sprite_addr[V[{x:X}]]"
    elif opcode & 0xF0FF == 0xF033:
        return f"set_BCD(V[{x:X}])"
    elif opcode & 0xF0FF == 0xF055:
        return f"mem[I..I+{x:X}] = V[0..{x:X}]"
    elif opcode & 0xF0FF == 0xF065:
        return f"V[0..{x:X}] = mem[I..I+{x:X}]"
    else:
        return "UNKNOWN"

args = sys.argv[1:]

with open(args[0], "rb") as f:
    data = f.read()

for i in range(0, len(data), 2):
    opcode = (data[i] << 8) | data[i + 1]
    print(f"{i+0x200:04x}: {decode_instruction(opcode)}")
