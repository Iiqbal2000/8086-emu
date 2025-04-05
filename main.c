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
    if (r_m != 0b110) {
        addr_str = strdup(reg_mem_map[r_m]);
    } else {
        int16_t disp16 = (int16_t)((disp_bytes[1] << 8) | disp_bytes[0]);
        int size = snprintf(NULL, 0, "%d", abs(disp16));
        addr_str = malloc(size + 1);
        snprintf(addr_str, size + 1, "%d", abs(disp16));
    }
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

      if (mod == 0b11) {
        const char *src = d_bit ? get_register(r_m, w_bit) : reg_name;
        const char *dst = d_bit ? reg_name : get_register(r_m, w_bit);
        printf("mov %s, %s\n", dst, src);
      } else if (mod == 0b00) {
        uint8_t *disp_bytes = malloc(sizeof(uint8_t) * 2);
          if (r_m == 0b110) {
            uint8_t disp_lo, disp_hi;
            read_byte(&disp_lo, file);
            read_byte(&disp_hi, file);

            disp_bytes[0] = disp_lo;
            disp_bytes[1] = disp_hi;
        }
        if (d_bit == 0b1) {
          printf("mov %s, [%s]\n", get_register(reg, w_bit),
                 get_memory_addressing(mod, r_m, disp_bytes));
        } else {
          printf("mov [%s], %s\n", get_memory_addressing(mod, r_m, disp_bytes),
                 get_register(reg, w_bit));
        }

        free(disp_bytes);
      } else if (mod == 0b01) { // displacement 8 bit
        uint8_t disp_lo;
        read_byte(&disp_lo, file);
        uint8_t *disp_bytes = malloc(sizeof(uint8_t));
        disp_bytes[0] = disp_lo;

        if (d_bit == 0b1) {
          printf("mov %s, [%s]\n", get_register(reg, w_bit),
                 get_memory_addressing(mod, r_m, disp_bytes));
        } else {
          printf("mov [%s], %s\n", get_memory_addressing(mod, r_m, disp_bytes),
                 get_register(reg, w_bit));
        }

        free(disp_bytes);
      } else if (mod == 0b10) {
        uint8_t disp_lo, disp_hi;
        read_byte(&disp_lo, file);
        read_byte(&disp_hi, file);

        uint8_t *disp_bytes = malloc(sizeof(uint8_t) * 2);
        disp_bytes[0] = disp_lo;
        disp_bytes[1] = disp_hi;

        if (d_bit == 0b1) {
          printf("mov %s, [%s]\n", get_register(reg, w_bit),
                 get_memory_addressing(mod, r_m, disp_bytes));
        } else {
          printf("mov [%s], %s\n", get_memory_addressing(mod, r_m, disp_bytes),
                 get_register(reg, w_bit));
        }

        free(disp_bytes);
      }

    } else if ((instruction & 0b11110000) >> 4 == 0b1011) { // immediate to register
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
    } else if ((instruction & 0b11111110) >> 1 == 0b1100011) {
        uint8_t w_bit = instruction & 0b00000001;
        read_byte(&instruction, file);
        // 2 bit for mod
        uint8_t mod = (instruction & 0b11000000) >> 6;
        // 3 bit for r_m
        uint8_t r_m = (instruction & 0b00000111);

        if (mod == 0b00) {
            if (w_bit == 0b0) {
                uint8_t data_byte;
                read_byte(&data_byte, file);
                printf("mov [%s], byte %d\n", get_memory_addressing(mod, r_m, NULL),
                    data_byte);
            } else {
                uint8_t data_lo, data_hi;
                read_byte(&data_lo, file);
                read_byte(&data_hi, file);

                int16_t data_word = (int16_t)((data_hi << 8) | data_lo);
                printf("mov [%s], word %d\n", get_memory_addressing(mod, r_m, NULL),
                       data_word);

            }
        } else if (mod == 0b01) {
            uint8_t disp_lo;
            read_byte(&disp_lo, file);
            uint8_t *disp_bytes = malloc(sizeof(uint8_t));
            disp_bytes[0] = disp_lo;

            if (w_bit == 0b0) {
                uint8_t data_byte;
                read_byte(&data_byte, file);
                printf("mov [%s], byte %d\n", get_memory_addressing(mod, r_m, disp_bytes),
                    data_byte);
            } else {
                uint8_t data_lo, data_hi;
                read_byte(&data_lo, file);
                read_byte(&data_hi, file);

                int16_t data_word = (int16_t)((data_hi << 8) | data_lo);
                printf("mov [%s], word %d\n", get_memory_addressing(mod, r_m, disp_bytes),
                       data_word);
            }

            free(disp_bytes);
        } else if (mod == 0b10) {
            uint8_t disp_lo, disp_hi;
            read_byte(&disp_lo, file);
            read_byte(&disp_hi, file);

            uint8_t *disp_bytes = malloc(sizeof(uint8_t) * 2);
            disp_bytes[0] = disp_lo;
            disp_bytes[1] = disp_hi;

            if (w_bit == 0b0) {
                uint8_t data_byte;
                read_byte(&data_byte, file);
                printf("mov [%s], byte %d\n", get_memory_addressing(mod, r_m, disp_bytes),
                    data_byte);
            } else {
                uint8_t data_lo, data_hi;
                read_byte(&data_lo, file);
                read_byte(&data_hi, file);

                int16_t data_word = (int16_t)((data_hi << 8) | data_lo);
                printf("mov [%s], word %d\n", get_memory_addressing(mod, r_m, disp_bytes),
                       data_word);
            }

            free(disp_bytes);
        }

    } else if ((instruction & 0b11111110) >> 1 == 0b1010000) {
        uint8_t w_bit = instruction & 0b00000001;

        uint8_t disp_lo, disp_hi;
        read_byte(&disp_lo, file);
        read_byte(&disp_hi, file);

        char *addr_str;

        int16_t disp16 = (int16_t)((disp_hi << 8) | disp_lo);
        printf("mov ax, [%d]\n", disp16);
    } else if ((instruction & 0b11111110) >> 1 == 0b1010001) {
        uint8_t w_bit = instruction & 0b00000001;

        uint8_t disp_lo, disp_hi;
        read_byte(&disp_lo, file);
        read_byte(&disp_hi, file);

        int16_t disp16 = (int16_t)((disp_hi << 8) | disp_lo);
        printf("mov [%d], ax\n", disp16);
    }
  }

  fclose(file);

  return 0;
}
