// SPDX-License-Identifier: GPL-2.0-only
/*
 * ggb/ggb.c
 * 
 * A small Game Boy emulator I'm working on.
 *
 * Copyright (C) 2025 Goldside543
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Flag bit masks
#define FLAG_Z  0x80 // Zero flag
#define FLAG_N  0x40 // Subtract flag
#define FLAG_H  0x20 // Half Carry flag
#define FLAG_C  0x10 // Carry flag

// Interrupt bits (IF and IE bits)
#define INT_VBLANK  0x01
#define INT_LCDSTAT 0x02
#define INT_TIMER   0x04
#define INT_SERIAL  0x08
#define INT_JOYPAD  0x10

// Game Boy CPU registers
typedef struct {
    union {
        struct {
            uint8_t f; // flags register
            uint8_t a; // accumulator
        };
        uint16_t af;
    };
    union {
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };
    union {
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };
    union {
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };
    uint16_t sp; // stack pointer
    uint16_t pc; // program counter
    bool halted;
    bool ime; // Interrupt Master Enable flag
} CPU;

uint8_t memory[0x10000];

// Special IO registers for interrupts
uint8_t *REG_IF = &memory[0xFF0F]; // Interrupt Flag
uint8_t *REG_IE = &memory[0xFFFF]; // Interrupt Enable

static inline void set_flag(CPU *cpu, uint8_t flag, bool condition) {
    if (condition) cpu->f |= flag;
    else cpu->f &= ~flag;
}

// Opcodes

void opcode_NOP(CPU *cpu) {
    printf("NOP executed at PC=0x%04X\n", cpu->pc - 1);
}

void opcode_HALT(CPU *cpu) {
    cpu->halted = true;
    printf("HALT executed at PC=0x%04X\n", cpu->pc - 1);
}

void opcode_STOP(CPU *cpu) {
    uint8_t next_byte = memory[cpu->pc++]; // fetch and ignore
    (void)next_byte;

    printf("STOP executed at PC=0x%04X\n", cpu->pc - 2);
    cpu->halted = true;  // treat like HALT for now
}

void opcode_LD_B_n(CPU *cpu) {
    uint8_t val = memory[cpu->pc++];
    cpu->b = val;
    printf("LD B, 0x%02X executed at PC=0x%04X\n", val, cpu->pc - 2);
}

void opcode_LD_A_n(CPU *cpu) {
    uint8_t val = memory[cpu->pc++];
    cpu->a = val;
    printf("LD A, 0x%02X executed at PC=0x%04X\n", val, cpu->pc - 2);
}

void opcode_LD_C_n(CPU *cpu) {
    uint8_t val = memory[cpu->pc++];
    cpu->c = val;
    printf("LD C, 0x%02X executed at PC=0x%04X\n", val, cpu->pc - 2);
}

void opcode_ADD_A_B(CPU *cpu) {
    uint8_t a = cpu->a;
    uint8_t b = cpu->b;
    uint16_t result = a + b;

    cpu->a = (uint8_t)result;

    set_flag(cpu, FLAG_Z, cpu->a == 0);
    set_flag(cpu, FLAG_N, false);
    set_flag(cpu, FLAG_H, ((a & 0xF) + (b & 0xF)) > 0xF);
    set_flag(cpu, FLAG_C, result > 0xFF);

    printf("ADD A, B executed: A=0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

void opcode_ADD_A_C(CPU *cpu) {
    uint8_t a = cpu->a;
    uint8_t c = cpu->c;
    uint16_t result = a + c;

    cpu->a = (uint8_t)result;

    set_flag(cpu, FLAG_Z, cpu->a == 0);
    set_flag(cpu, FLAG_N, false);
    set_flag(cpu, FLAG_H, ((a & 0xF) + (c & 0xF)) > 0xF);
    set_flag(cpu, FLAG_C, result > 0xFF);

    printf("ADD A, C executed: A=0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

void opcode_LD_D_n(CPU *cpu) {
    uint8_t val = memory[cpu->pc++];
    cpu->d = val;
    printf("LD D, 0x%02X executed at PC=0x%04X\n", val, cpu->pc - 2);
}

void opcode_LD_E_n(CPU *cpu) {
    uint8_t val = memory[cpu->pc++];
    cpu->e = val;
    printf("LD E, 0x%02X executed at PC=0x%04X\n", val, cpu->pc - 2);
}

void opcode_LD_H_n(CPU *cpu) {
    uint8_t val = memory[cpu->pc++];
    cpu->h = val;
    printf("LD H, 0x%02X executed at PC=0x%04X\n", val, cpu->pc - 2);
}

void opcode_LD_L_n(CPU *cpu) {
    uint8_t val = memory[cpu->pc++];
    cpu->l = val;
    printf("LD L, 0x%02X executed at PC=0x%04X\n", val, cpu->pc - 2);
}

void opcode_INC_B(CPU *cpu) {
    cpu->b++;
    set_flag(cpu, FLAG_Z, cpu->b == 0);
    set_flag(cpu, FLAG_N, false);
    set_flag(cpu, FLAG_H, (cpu->b & 0x0F) == 0x00);
    printf("INC B executed: B=0x%02X at PC=0x%04X\n", cpu->b, cpu->pc - 1);
}

void opcode_DEC_B(CPU *cpu) {
    set_flag(cpu, FLAG_H, (cpu->b & 0x0F) == 0x00);
    cpu->b--;
    set_flag(cpu, FLAG_Z, cpu->b == 0);
    set_flag(cpu, FLAG_N, true);
    printf("DEC B executed: B=0x%02X at PC=0x%04X\n", cpu->b, cpu->pc - 1);
}

void opcode_AND_A_B(CPU *cpu) {
    cpu->a &= cpu->b;
    set_flag(cpu, FLAG_Z, cpu->a == 0);
    set_flag(cpu, FLAG_N, false);
    set_flag(cpu, FLAG_H, true);
    set_flag(cpu, FLAG_C, false);
    printf("AND A, B executed: A=0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

void opcode_XOR_A_A(CPU *cpu) {
    cpu->a ^= cpu->a;
    set_flag(cpu, FLAG_Z, cpu->a == 0);
    set_flag(cpu, FLAG_N, false);
    set_flag(cpu, FLAG_H, false);
    set_flag(cpu, FLAG_C, false);
    printf("XOR A, A executed: A=0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

void opcode_JP_nn(CPU *cpu) {
    uint16_t addr = memory[cpu->pc] | (memory[cpu->pc + 1] << 8);
    cpu->pc = addr;
    printf("JP to 0x%04X\n", addr);
}

void opcode_CALL_nn(CPU *cpu) {
    uint16_t addr = memory[cpu->pc] | (memory[cpu->pc + 1] << 8);
    cpu->pc += 2;
    push_stack(cpu, cpu->pc);
    cpu->pc = addr;
    printf("CALL to 0x%04X\n", addr);
}

void opcode_RET(CPU *cpu) {
    uint16_t lo = memory[cpu->sp++];
    uint16_t hi = memory[cpu->sp++];
    cpu->pc = lo | (hi << 8);
    printf("RET to 0x%04X\n", cpu->pc);
}

void opcode_LD_HL_A(CPU *cpu) {
    memory[cpu->hl] = cpu->a;
    printf("LD (HL), A executed: HL=0x%04X <- A=0x%02X at PC=0x%04X\n", cpu->hl, cpu->a, cpu->pc - 1);
}

void opcode_LD_A_HL(CPU *cpu) {
    cpu->a = memory[cpu->hl];
    printf("LD A, (HL) executed: A <- (0x%04X)=0x%02X at PC=0x%04X\n", cpu->hl, cpu->a, cpu->pc - 1);
}

void opcode_LD_a16_A(CPU *cpu) {
    uint16_t addr = memory[cpu->pc] | (memory[cpu->pc + 1] << 8);
    cpu->pc += 2;
    memory[addr] = cpu->a;
    printf("LD (0x%04X), A executed: A=0x%02X at PC=0x%04X\n", addr, cpu->a, cpu->pc - 3);
}

void opcode_LD_A_a16(CPU *cpu) {
    uint16_t addr = memory[cpu->pc] | (memory[cpu->pc + 1] << 8);
    cpu->pc += 2;
    cpu->a = memory[addr];
    printf("LD A, (0x%04X) executed: A=0x%02X at PC=0x%04X\n", addr, cpu->a, cpu->pc - 3);
}

void opcode_LD_C_A(CPU *cpu) {
    memory[0xFF00 + cpu->c] = cpu->a;
    printf("LD (0xFF00+C), A executed: [0x%04X] = 0x%02X at PC=0x%04X\n", 0xFF00 + cpu->c, cpu->a, cpu->pc - 1);
}

void opcode_LD_A_C(CPU *cpu) {
    cpu->a = memory[0xFF00 + cpu->c];
    printf("LD A, (0xFF00+C) executed: A = [0x%04X] = 0x%02X at PC=0x%04X\n", 0xFF00 + cpu->c, cpu->a, cpu->pc - 1);
}

void opcode_LD_FF00_n_A(CPU *cpu) {
    uint8_t offset = memory[cpu->pc++];
    memory[0xFF00 + offset] = cpu->a;
    printf("LD (0xFF00+0x%02X), A executed: [0x%04X] = 0x%02X at PC=0x%04X\n", offset, 0xFF00 + offset, cpu->a, cpu->pc - 2);
}

void opcode_LD_A_FF00_n(CPU *cpu) {
    uint8_t offset = memory[cpu->pc++];
    cpu->a = memory[0xFF00 + offset];
    printf("LD A, (0xFF00+0x%02X) executed: A = 0x%02X at PC=0x%04X\n", offset, cpu->a, cpu->pc - 2);
}

// 0x01 - LD BC, nn
void opcode_LD_BC_nn(CPU *cpu) {
    uint16_t nn = memory[cpu->pc] | (memory[cpu->pc + 1] << 8);
    cpu->bc = nn;
    cpu->pc += 2;
    printf("LD BC, 0x%04X executed: BC = 0x%04X at PC=0x%04X\n", nn, cpu->bc, cpu->pc - 2);
}

// 0x09 - ADD HL, BC
void opcode_ADD_HL_BC(CPU *cpu) {
    uint16_t result = cpu->hl + cpu->bc;
    cpu->f = (cpu->hl & 0x8000) != (result & 0x8000);  // Set the carry flag if there's overflow
    cpu->hl = result;
    printf("ADD HL, BC executed: HL = 0x%04X at PC=0x%04X\n", cpu->hl, cpu->pc - 1);
}

// 0x21 - LD HL, nn
void opcode_LD_HL_nn(CPU *cpu) {
    uint16_t nn = memory[cpu->pc] | (memory[cpu->pc + 1] << 8);
    cpu->hl = nn;
    cpu->pc += 2;
    printf("LD HL, 0x%04X executed: HL = 0x%04X at PC=0x%04X\n", nn, cpu->hl, cpu->pc - 2);
}

// 0x31 - LD SP, nn
void opcode_LD_SP_nn(CPU *cpu) {
    uint16_t nn = memory[cpu->pc] | (memory[cpu->pc + 1] << 8);
    cpu->sp = nn;
    cpu->pc += 2;
    printf("LD SP, 0x%04X executed: SP = 0x%04X at PC=0x%04X\n", nn, cpu->sp, cpu->pc - 2);
}

// 0x3C - INC A
void opcode_INC_A(CPU *cpu) {
    cpu->a++;
    cpu->f = (cpu->a == 0) ? FLAG_Z : 0;  // Set Zero flag if A is 0
    printf("INC A executed: A = 0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

// 0x2F - CPL (Complement A)
void opcode_CPL(CPU *cpu) {
    cpu->a = ~cpu->a;
    cpu->f = FLAG_N | FLAG_H;  // Set Subtract and Half Carry flags
    printf("CPL executed: A = 0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

// 0xE6 - AND n
void opcode_AND_n(CPU *cpu) {
    uint8_t n = memory[cpu->pc++];
    cpu->a &= n;
    cpu->f = (cpu->a == 0) ? FLAG_Z : 0;  // Set Zero flag if A is 0
    cpu->f |= FLAG_H;  // Set Half Carry flag (since AND is a logical operation)
    printf("AND 0x%02X executed: A = 0x%02X at PC=0x%04X\n", n, cpu->a, cpu->pc - 1);
}

// 0xA7 - AND A
void opcode_AND_A(CPU *cpu) {
    cpu->a &= cpu->a;  // ANDing A with itself will just clear the non-zero bits
    cpu->f = (cpu->a == 0) ? FLAG_Z : 0;  // Set Zero flag if A is 0
    cpu->f |= FLAG_H;  // Set Half Carry flag (since AND is a logical operation)
    printf("AND A executed: A = 0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

// 0xA1 - XOR A, C
void opcode_XOR_A_C(CPU *cpu) {
    cpu->a ^= cpu->c;
    cpu->f = (cpu->a == 0) ? FLAG_Z : 0;
    printf("XOR A, C executed: A = 0x%02X at PC=0x%04X\n", cpu->a, cpu->pc - 1);
}

typedef void (*OpcodeFunc)(CPU *);

OpcodeFunc opcode_table[256] = {
    [0x00] = opcode_NOP,
    [0x01] = opcode_LD_BC_nn,
    [0x06] = opcode_LD_B_n,
    [0x09] = opcode_ADD_HL_BC,
    [0x0E] = opcode_LD_C_n,
    [0x16] = opcode_LD_D_n,
    [0x1E] = opcode_LD_E_n,
    [0x26] = opcode_LD_H_n,
    [0x2E] = opcode_LD_L_n,

    [0x04] = opcode_INC_B,
    [0x05] = opcode_DEC_B,

    [0x3E] = opcode_LD_A_n,
    [0x10] = opcode_STOP,
    [0x76] = opcode_HALT,

    [0x80] = opcode_ADD_A_B,
    [0x81] = opcode_ADD_A_C,
    [0xA0] = opcode_AND_A_B,
    [0xAF] = opcode_XOR_A_A,

    [0xC3] = opcode_JP_nn,
    [0xCD] = opcode_CALL_nn,
    [0xC9] = opcode_RET,
    
    [0x77] = opcode_LD_HL_A,
    [0x7E] = opcode_LD_A_HL,

    [0xEA] = opcode_LD_a16_A,
    [0xFA] = opcode_LD_A_a16,

    [0xE2] = opcode_LD_C_A,
    [0xF2] = opcode_LD_A_C,

    [0xE0] = opcode_LD_FF00_n_A,
    [0xF0] = opcode_LD_A_FF00_n,
    
    [0x21] = opcode_LD_HL_nn,
    [0x31] = opcode_LD_SP_nn,
    [0x3C] = opcode_INC_A,
    [0x2F] = opcode_CPL,
    [0xE6] = opcode_AND_n,
    [0xA7] = opcode_AND_A,
    [0xA1] = opcode_XOR_A_C,
};

// Push PC to stack helper (little endian)
void push_stack(CPU *cpu, uint16_t val) {
    cpu->sp--;
    memory[cpu->sp] = (val >> 8) & 0xFF;
    cpu->sp--;
    memory[cpu->sp] = val & 0xFF;
}

// Simple interrupt handler (only VBLANK for demo)
void handle_interrupts(CPU *cpu) {
    if (!cpu->ime) return; // interrupts disabled

    uint8_t fired = (*REG_IF) & (*REG_IE);
    if (fired == 0) return;

    cpu->halted = false; // wake CPU if halted

    // Prioritize interrupts low bit first (VBLANK)
    if (fired & INT_VBLANK) {
        *REG_IF &= ~INT_VBLANK; // clear IF flag
        cpu->ime = false; // disable further interrupts
        push_stack(cpu, cpu->pc);
        cpu->pc = 0x40; // VBLANK ISR address
        printf("Interrupt VBLANK handled! Jump to 0x0040\n");
        return;
    }
    // Add others later...
}

typedef struct {
    int mode;         // 0–3
    int mode_clock;   // cycles in current mode
    int line;         // current scanline (0–153)
} PPU;

PPU ppu;

void push_framebuffer_to_screen() {
  return; // Stub
}

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define TILE_SIZE 8

uint8_t framebuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

void draw_scanline(int line) {
    // Get scroll values from registers
    uint8_t scroll_y = memory[0xFF42];
    uint8_t scroll_x = memory[0xFF43];

    int y = (scroll_y + line) & 0xFF;      // vertical wrap in BG
    int tile_row = y / TILE_SIZE;

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int x_pos = (scroll_x + x) & 0xFF; // horizontal wrap in BG
        int tile_col = x_pos / TILE_SIZE;

        // BG Map base address (0x9800) minus VRAM start 0x8000 = offset 0x1800
        uint16_t bg_map_offset = 0x1800 + tile_row * 32 + tile_col;

        // Read tile index from BG Map
        uint8_t tile_index = memory[0x8000 + bg_map_offset];

        // Tile data base address at 0x8000, each tile 16 bytes
        uint16_t tile_data_offset = tile_index * 16;
        int line_in_tile = y % TILE_SIZE;

        // Read tile line data
        uint8_t byte1 = memory[0x8000 + tile_data_offset + line_in_tile * 2];
        uint8_t byte2 = memory[0x8000 + tile_data_offset + line_in_tile * 2 + 1];

        // Calculate bit index for pixel in tile
        int bit = 7 - (x_pos % TILE_SIZE);

        // Get color number from bit planes
        int color_num = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

        // Store color number in your framebuffer (or palette index)
        framebuffer[line][x] = color_num;
    }
}

#define OAM_START 0xFE00
#define SPRITE_ATTRS 4
#define MAX_SPRITES 40
#define SPRITE_HEIGHT 8  // 8 or 16 depending on LCDC bit

void draw_sprites_on_scanline(int line) {
    for (int i = 0; i < MAX_SPRITES; i++) {
        int base = OAM_START + i * SPRITE_ATTRS;
        int sprite_y = memory[base] - 16;
        int sprite_x = memory[base + 1] - 8;
        uint8_t tile_index = memory[base + 2];
        uint8_t attributes = memory[base + 3];

        if (line < sprite_y || line >= sprite_y + SPRITE_HEIGHT)
            continue; // sprite not on this scanline

        int line_in_sprite = line - sprite_y;

        // Flip Y if needed
        if (attributes & 0x40) {
            line_in_sprite = SPRITE_HEIGHT - 1 - line_in_sprite;
        }

        // Tile data offset for this line
        uint16_t tile_data_offset = tile_index * 16 + line_in_sprite * 2;

        uint8_t byte1 = memory[0x8000 + tile_data_offset];
        uint8_t byte2 = memory[0x8000 + tile_data_offset + 1];

        for (int x = 0; x < 8; x++) {
            int bit = 7 - x;

            // Flip X if needed
            int pixel_bit = (attributes & 0x20) ? x : bit;

            int color_num = ((byte2 >> pixel_bit) & 1) << 1 | ((byte1 >> pixel_bit) & 1);
            if (color_num == 0) continue; // transparent pixel

            int pixel_x = sprite_x + x;
            if (pixel_x < 0 || pixel_x >= SCREEN_WIDTH) continue;

            // Choose palette 0 or 1
            uint8_t palette = (attributes & 0x10) ? memory[0xFF49] : memory[0xFF48];

            // Map color_num through palette (2 bits per color)
            int shade = (palette >> (color_num * 2)) & 0x3;

            // TODO: Respect priority and BG color zero rules

            framebuffer[line][pixel_x] = shade;
        }
    }
}

void ppu_step(int cycles) {
    ppu.mode_clock += cycles;

    switch (ppu.mode) {
        case 2: // OAM scan
            if (ppu.mode_clock >= 80) {
                ppu.mode_clock -= 80;
                ppu.mode = 3;
            }
            break;
        case 3: // Drawing
            if (ppu.mode_clock >= 172) {
                ppu.mode_clock -= 172;
                ppu.mode = 0;
                // draw the scanline
                draw_scanline(ppu.line);
                draw_sprites_on_scanline(ppu.line);
            }
            break;
        case 0: // H-Blank
            if (ppu.mode_clock >= 204) {
                ppu.mode_clock -= 204;
                ppu.line++;
                if (ppu.line == 144) {
                    ppu.mode = 1; // V-Blank
                    // trigger V-Blank interrupt
                    *REG_IF |= INT_VBLANK;
                    // update framebuffer
                    push_framebuffer_to_screen();
                } else {
                    ppu.mode = 2;
                }
            }
            break;
        case 1: // V-Blank
            if (ppu.mode_clock >= 456) {
                ppu.mode_clock -= 456;
                ppu.line++;
                if (ppu.line > 153) {
                    ppu.mode = 2;
                    ppu.line = 0;
                }
            }
            break;
    }
}

void cpu_execute_instruction(CPU *cpu) {
    handle_interrupts(cpu);

    if (cpu->halted) {
        // CPU halted: do nothing except wait for interrupt
        return;
    }

    uint8_t opcode = memory[cpu->pc++];
    if (opcode_table[opcode]) {
        opcode_table[opcode](cpu);
    } else {
        printf("Unknown opcode 0x%02X at PC=0x%04X\n", opcode, cpu->pc - 1);
    }
}

void load_fake_boot(CPU *cpu) {
    cpu->a = 0x01;
    cpu->f = 0xB0;
    cpu->b = 0x00;
    cpu->c = 0x13;
    cpu->d = 0x00;
    cpu->e = 0xD8;
    cpu->h = 0x01;
    cpu->l = 0x4D;
    cpu->sp = 0xFFFE;
    cpu->pc = 0x0100; // Skip boot ROM, jump straight to cartridge start
    cpu->ime = true;

    memory[0xFF05] = 0x00; // TIMA
    memory[0xFF06] = 0x00; // TMA
    memory[0xFF07] = 0x00; // TAC
    memory[0xFF10] = 0x80; // NR10
    memory[0xFF11] = 0xBF; // NR11
    memory[0xFF12] = 0xF3; // NR12
    memory[0xFF14] = 0xBF; // NR14
    memory[0xFF16] = 0x3F; // NR21
    memory[0xFF17] = 0x00; // NR22
    memory[0xFF19] = 0xBF; // NR24
    memory[0xFF1A] = 0x7F; // NR30
    memory[0xFF1B] = 0xFF; // NR31
    memory[0xFF1C] = 0x9F; // NR32
    memory[0xFF1E] = 0xBF; // NR33
    memory[0xFF20] = 0xFF; // NR41
    memory[0xFF21] = 0x00; // NR42
    memory[0xFF22] = 0x00; // NR43
    memory[0xFF23] = 0xBF; // NR44
    memory[0xFF24] = 0x77; // NR50
    memory[0xFF25] = 0xF3; // NR51
    memory[0xFF26] = 0xF1; // NR52 (GB) or 0xF0 (GBC)
    memory[0xFF40] = 0x91; // LCDC
    memory[0xFF42] = 0x00; // SCY
    memory[0xFF43] = 0x00; // SCX
    memory[0xFF45] = 0x00; // LYC
    memory[0xFF47] = 0xFC; // BGP
    memory[0xFF48] = 0xFF; // OBP0
    memory[0xFF49] = 0xFF; // OBP1
    memory[0xFF4A] = 0x00; // WY
    memory[0xFF4B] = 0x00; // WX
    memory[0xFFFF] = 0x00; // IE

    // Clear WRAM for consistency
    for (uint16_t i = 0xC000; i <= 0xDFFF; i++) {
        memory[i] = 0x00;
    }
}

int main() {
    // Set up CPU with interrupts enabled and stack pointer somewhere safe
    CPU cpu;
    
    load_fake_boot(&cpu);

    // Enable VBLANK interrupt only for demo
    *REG_IE = INT_VBLANK;

    // Test program
    memory[0x100] = 0x3E; // LD A, n
    memory[0x101] = 0x0A; // A = 0x0A
    memory[0x102] = 0x06; // LD B, n
    memory[0x103] = 0x05; // B = 0x05
    memory[0x104] = 0x80; // ADD A, B  (A=0x0A+0x05=0x0F)
    memory[0x105] = 0xC3; // JP
    memory[0x106] = 0x08; // second part address to jump to
    memory[0x107] = 0x01; // first part of address to jump to
    memory[0x108] = 0x76; // HALT

    int cycles = 0;

    while (!cpu.halted) {
        cpu_execute_instruction(&cpu);
        cycles++;
    }

    printf("Emulation finished.\n");
    return 0;
}
