#include <stdint.h>
#include <stdio.h>

int main(void) {
  FILE *file;

  file = fopen("many_register_mov", "rb");

  // little-endian byte order
  uint16_t instruction;

  while (fread(&instruction, sizeof(instruction), 1, file) == 1) {
    uint8_t byte1 = instruction & 0b11111111;
    uint8_t byte2 = (instruction >> 8) & 0b11111111;

    uint8_t opcode = (byte1 & 0b11111100) >> 2;
    if (opcode == 0b100010) {
      // 1 bit for d
      uint8_t d_bit = (byte1 & 0b00000010) >> 1;
      // 1 bit for w
      uint8_t w_bit = byte1 & 0b00000001;

      // 2 bit for mod
      uint8_t mod = (byte2 & 0b11000000) >> 6;
      // 3 bit for reg
      uint8_t reg = (byte2 & 0b00111000) >> 3;
      // 3 bit for r_m
      uint8_t r_m = (byte2 & 0b00000111);

      if (mod == 0b11) {
        uint8_t src_index, dst_index;

        if (d_bit == 0b1) {
          dst_index = reg;
          src_index = r_m;
        } else {
          dst_index = r_m;
          src_index = reg;
        }

        if (w_bit == 0b1) {
          const char *reg_in_word[] = {"ax", "cx", "dx", "bx",
                                       "sp", "bp", "si", "di"};
          printf("mov %s, %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                 reg_in_word[dst_index], reg_in_word[src_index], byte1, w_bit,
                 d_bit, r_m, reg, mod);
        } else {
          const char *reg_in_byte[] = {"al", "cl", "dl", "bl",
                                       "ah", "ch", "dh", "bh"};
          printf("mov %s, %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n",
                 reg_in_byte[dst_index], reg_in_byte[src_index], byte1, w_bit,
                 d_bit, r_m, reg, mod);
        }
      }
    }
  }

  fclose(file);

  return 0;
}
