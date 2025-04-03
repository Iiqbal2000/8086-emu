#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

const char *reg_in_word[] = {"ax", "cx", "dx", "bx",
                             "sp", "bp", "si", "di"};

const char *reg_in_byte[] = {"al", "cl", "dl", "bl",
                             "ah", "ch", "dh", "bh"};

const char *reg_mem_map[] = {
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx"
};

void print_bit(uint8_t bits) {
    for (int bit = 7; bit >= 0; bit--) {
        putchar( ( (bits >> bit) & 1 ) ? '1' : '0' );
    }
    putchar(' ');
}

size_t read_byte(uint8_t *buffer, FILE *file) {
    size_t result = fread(buffer, sizeof(uint8_t), 1, file);
    print_bit(*buffer);
    return result;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("Need an argument!\n");
    return 0;
  }

  FILE *file;

  file = fopen(argv[1], "rb");

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

      // register mode
      if (mod == 0b11) {
        uint8_t src_index, dst_index;

        if (d_bit == 0b1) {
          dst_index = reg;
          src_index = r_m;
        } else {
          dst_index = r_m;
          src_index = reg;
        }

        // word size
        if (w_bit == 0b1) {
          printf("mov %s, %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                 reg_in_word[dst_index], reg_in_word[src_index], instruction, w_bit,
                 d_bit, r_m, reg, mod);
        } else {
          printf("mov %s, %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                 reg_in_byte[dst_index], reg_in_byte[src_index], instruction, w_bit,
                 d_bit, r_m, reg, mod);
        }
      } else if (mod == 0b00) {
          if (d_bit == 0b1) {
            if (w_bit == 0) {
                printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                         reg_in_byte[reg], reg_mem_map[r_m], instruction, w_bit,
                         d_bit, r_m, reg, mod);
            } else {
                printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                         reg_in_word[reg], reg_mem_map[r_m], instruction, w_bit,
                         d_bit, r_m, reg, mod);
            }
          } else {
              if (w_bit == 0) {
                  printf("mov [%s], %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                           reg_mem_map[r_m], reg_in_byte[reg], instruction, w_bit,
                           d_bit, r_m, reg, mod);
              } else {
                  printf("mov [%s], %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                        reg_mem_map[r_m], reg_in_word[reg], instruction, w_bit,
                           d_bit, r_m, reg, mod);
              }
          }
      } else if (mod == 0b01) { // displacement 8 bit
          read_byte(&instruction, file);
          uint8_t disp_lo = instruction;

          if (d_bit == 0b1) {
            if (w_bit == 0) {
                if ((disp_lo & disp_lo) == 0) {
                    printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                             reg_in_byte[reg], reg_mem_map[r_m], instruction, w_bit,
                             d_bit, r_m, reg, mod);
                } else {
                    char r_m_str[] = "%s + %d";
                    int size = snprintf(NULL, 0, r_m_str, reg_mem_map[r_m], disp_lo);

                    if (size < 0) {
                        fprintf(stderr, "snprintf failed\n");
                        return 1; // Or handle the error appropriately
                    }

                    char* formatted_r_m_str = (char*)malloc(size + 1); // +1 for null terminator
                    if (formatted_r_m_str == NULL) {
                        fprintf(stderr, "Memory allocation failed\n");
                        return 1; // Or handle the error appropriately
                    }

                    snprintf(formatted_r_m_str, size + 1, r_m_str, reg_mem_map[r_m], disp_lo);

                    printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                             reg_in_byte[reg], formatted_r_m_str, instruction, w_bit,
                             d_bit, r_m, reg, mod);

                    free(formatted_r_m_str);
                }
            } else {
                printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                         reg_in_word[reg], reg_mem_map[r_m], instruction, w_bit,
                         d_bit, r_m, reg, mod);
            }
          } else {
              if (w_bit == 0) {
                  printf("mov [%s], %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                           reg_mem_map[r_m], reg_in_byte[reg], instruction, w_bit,
                           d_bit, r_m, reg, mod);
              } else {
                  printf("mov [%s], %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                        reg_mem_map[r_m], reg_in_word[reg], instruction, w_bit,
                           d_bit, r_m, reg, mod);
              }
          }
      } else if (mod == 0b10) {
          read_byte(&instruction, file);
          uint8_t disp_lo = instruction;
          read_byte(&instruction, file);
          uint8_t disp_hi = instruction;

          uint16_t disp_16_bits = (disp_hi << 8) | disp_lo;

          if (d_bit == 0b1) {
            if (w_bit == 0) {
                if ((disp_lo & disp_lo) == 0) {
                    printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                             reg_in_byte[reg], reg_mem_map[r_m], instruction, w_bit,
                             d_bit, r_m, reg, mod);
                } else {
                    char r_m_str[] = "%s + %d";
                    int size = snprintf(NULL, 0, r_m_str, reg_mem_map[r_m], disp_16_bits);

                    if (size < 0) {
                        fprintf(stderr, "snprintf failed\n");
                        return 1; // Or handle the error appropriately
                    }

                    char* formatted_r_m_str = (char*)malloc(size + 1); // +1 for null terminator
                    if (formatted_r_m_str == NULL) {
                        fprintf(stderr, "Memory allocation failed\n");
                        return 1; // Or handle the error appropriately
                    }

                    snprintf(formatted_r_m_str, size + 1, r_m_str, reg_mem_map[r_m], disp_16_bits);

                    printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                             reg_in_byte[reg], formatted_r_m_str, instruction, w_bit,
                             d_bit, r_m, reg, mod);

                    free(formatted_r_m_str);
                }
            } else {
                printf("mov %s, [%s] ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                         reg_in_word[reg], reg_mem_map[r_m], instruction, w_bit,
                         d_bit, r_m, reg, mod);
            }
          } else {
              if (w_bit == 0) {
                  printf("mov [%s], %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                           reg_mem_map[r_m], reg_in_byte[reg], instruction, w_bit,
                           d_bit, r_m, reg, mod);
              } else {
                  printf("mov [%s], %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                        reg_mem_map[r_m], reg_in_word[reg], instruction, w_bit,
                           d_bit, r_m, reg, mod);
              }
          }
      }

    } else if ((instruction & 0b11110000) >> 4 == 0b1011) {
        // extract high 4 bits
        // opcode = (instruction & 0b11110000) >> 4;
        // print_bit(opcode);
        uint8_t w_bit = (instruction & 0b00001000) >> 3;
        uint8_t reg = instruction & 0b00000111;

        if (w_bit == 0b1) {
          read_byte(&instruction, file);
          uint8_t byte1 = instruction;
          read_byte(&instruction, file);
          uint8_t byte2 = instruction;

          // join byte1 and byte2
          uint16_t upper_16_bits = (byte2 << 8) | byte1;

          printf("mov %s, %d ;\n", reg_in_word[reg], upper_16_bits);
        } else {
          read_byte(&instruction, file);

          printf("mov %s, %d ;\n", reg_in_byte[reg], instruction);
        }
    }


  }

  fclose(file);

  return 0;
}
