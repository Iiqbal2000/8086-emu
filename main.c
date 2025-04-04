#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *reg_in_word[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

const char *reg_in_byte[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

const char *reg_mem_map[] = {"bx + si", "bx + di", "bp + si", "bp + di",
                             "si",      "di",      "bp",      "bx"};

void print_bit(uint8_t bits) {
  for (int bit = 7; bit >= 0; bit--) {
    putchar(((bits >> bit) & 1) ? '1' : '0');
  }
  putchar(' ');
}

size_t read_byte(uint8_t *buffer, FILE *file) {
  size_t result = fread(buffer, sizeof(uint8_t), 1, file);
  // print_bit(*buffer);
  return result;
}

const char *get_register(uint8_t reg, uint8_t w_bit) {
    return w_bit ? reg_in_word[reg] : reg_in_byte[reg];
}

char *get_memory_addressing(uint8_t mod, uint8_t r_m, uint8_t *disp_bytes) {
  char *addr_str;
  if (mod == 0b00) {
    addr_str = strdup(reg_mem_map[r_m]);
  } else if (mod == 0b01) {
    int8_t disp8 = (int8_t)disp_bytes[0];
    int size = snprintf(NULL, 0, "%s %s %d", reg_mem_map[r_m],
                        disp8 >= 0 ? "+" : "-", abs(disp8));
    addr_str = malloc(size + 1);
    snprintf(addr_str, size + 1, "%s %s %d", reg_mem_map[r_m],
             disp8 >= 0 ? "+" : "-", abs(disp8));
  } else if (mod == 0b10) {
    int16_t disp16 = (int16_t)((disp_bytes[1] << 8) | disp_bytes[0]);
    int size = snprintf(NULL, 0, "%s %s %d", reg_mem_map[r_m],
                        disp16 >= 0 ? "+" : "-", abs(disp16));
    addr_str = malloc(size + 1);
    snprintf(addr_str, size + 1, "%s %s %d", reg_mem_map[r_m],
             disp16 >= 0 ? "+" : "-", abs(disp16));
  } else {           // mod == 0b11
    addr_str = NULL; // Register mode, no memory addressing
  }
  return addr_str;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <file>\n", argv[0]);
    return 1;
  }

  FILE *file = fopen(argv[1], "rb");
  if (!file) {
    perror("Failed to open file");
    return 1;
  }

  // little-endian byte order
  uint8_t instruction;

  while (read_byte(&instruction, file)) {
    // extract high 6 bits
    uint8_t opcode = (instruction & 0b11111100) >> 2;
    if (opcode == 0b100010) {
      // 1 bit for d
      uint8_t d_bit = (instruction & 0b00000010) >> 1;
      // 1 bit for w
      uint8_t w_bit = instruction & 0b00000001;

      read_byte(&instruction, file);

      // 2 bit for mod
      uint8_t mod = (instruction & 0b11000000) >> 6;
      // 3 bit for reg
      uint8_t reg = (instruction & 0b00111000) >> 3;
      // 3 bit for r_m
      uint8_t r_m = (instruction & 0b00000111);

      const char *reg_name = get_register(reg, w_bit);

      // register mode
      if (mod == 0b11) {
        const char *src = d_bit ? get_register(r_m, w_bit) : reg_name;
        const char *dst = d_bit ? reg_name : get_register(r_m, w_bit);
        printf("mov %s, %s\n", dst, src);

      } else if (mod == 0b00) {
        if (d_bit == 0b1) {
            printf("mov %s, [%s]\n",
                   get_register(reg, w_bit), get_memory_addressing(mod, r_m, NULL));
        } else {
            printf("mov [%s], %s\n",
                   get_memory_addressing(mod, r_m, NULL), get_register(reg, w_bit));
        }
      } else if (mod == 0b01) { // displacement 8 bit
        read_byte(&instruction, file);
        uint8_t disp_lo = instruction;
        uint8_t *disp_bytes = malloc(sizeof(uint8_t));
        disp_bytes[0] = disp_lo;

        if (d_bit == 0b1) {
            printf("mov %s, [%s]\n",
                   get_register(reg, w_bit), get_memory_addressing(mod, r_m, disp_bytes));
        } else {
            printf("mov [%s], %s\n",
                   get_memory_addressing(mod, r_m, disp_bytes), get_register(reg, w_bit));
        }

        free(disp_bytes);

      } else if (mod == 0b10) {
        read_byte(&instruction, file);
        uint8_t disp_lo = instruction;
        read_byte(&instruction, file);
        uint8_t disp_hi = instruction;

        uint8_t *disp_bytes = malloc(sizeof(uint8_t) * 2);
        disp_bytes[0] = disp_lo;
        disp_bytes[1] = disp_hi;

        if (d_bit == 0b1) {
            printf("mov %s, [%s]\n",
                   get_register(reg, w_bit), get_memory_addressing(mod, r_m, disp_bytes));
        } else {
            printf("mov [%s], %s\n",
                   get_memory_addressing(mod, r_m, disp_bytes), get_register(reg, w_bit));
        }

        free(disp_bytes);
      }

    } else if ((instruction & 0b11110000) >> 4 == 0b1011) {
      // extract high 4 bits
      uint8_t w_bit = (instruction & 0b00001000) >> 3;
      uint8_t reg = instruction & 0b00000111;

      if (w_bit == 0b1) {
        read_byte(&instruction, file);
        uint8_t byte1 = instruction;
        read_byte(&instruction, file);
        uint8_t byte2 = instruction;

        // join byte1 and byte2
        uint16_t upper_16_bits = (byte2 << 8) | byte1;

        printf("mov %s, %d\n", reg_in_word[reg], upper_16_bits);
      } else {
        read_byte(&instruction, file);

        printf("mov %s, %d\n", reg_in_byte[reg], instruction);
      }
    }
  }

  fclose(file);

  return 0;
}
