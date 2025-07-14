import sys


def bits_to_str(byte):
    str = "".join("1" if (byte & (1 << (7 - i))) else " " for i in range(8))
    return str


def decode_instruction(opcode, ram=None, I=0):
    nnn = opcode & 0x0FFF
    n = opcode & 0x000F
    x = (opcode & 0x0F00) >> 8
    y = (opcode & 0x00F0) >> 4
    kk = opcode & 0x00FF

    if opcode == 0x00E0:
        return "disp_clear()"
    elif opcode == 0x00EE:
        return "return"
    elif opcode & 0xF000 == 0x1000:
        return f"goto 0x{nnn:03X}"
    elif opcode & 0xF000 == 0x2000:
        return f"call 0x{nnn:03X}"
    elif opcode & 0xF000 == 0x6000:
        return f"V[{x:X}] = 0x{kk:02X}"
    elif opcode & 0xF000 == 0x7000:
        return f"V[{x:X}] += 0x{kk:02X}"
    elif opcode & 0xF000 == 0xA000:
        return f"I = 0x{nnn:03X}"
    elif opcode & 0xF000 == 0xD000:
        info = f"draw(V[{x:X}], V[{y:X}], 0x{n:X})"
        if ram is not None:
            offset = I - 0x200
            sprite_bytes = ram[offset : offset + n]
            sprite_visual = "\n  " + "\n  ".join(
                bits_to_str(b) for b in sprite_bytes
            )
            return info + "\n  Sprite visual:\n" + sprite_visual + '\n'
        return info
    else:
        return None


args = sys.argv[1:]

with open(args[0], "rb") as f:
    data = f.read()

PC = 0x200
I = 0

for i in range(0, len(data), 2):
    opcode = (data[i] << 8) | data[i + 1]

    if (opcode & 0xF000) == 0xA000:
        I = opcode & 0x0FFF

    decoded = decode_instruction(opcode, data, I)
    if decoded is not None:
        hex_bytes = f"{data[i]:02X} {data[i + 1]:02X}"
        print(f"{PC + i:04X}: {opcode:04X} | {decoded} <{hex_bytes}>")
