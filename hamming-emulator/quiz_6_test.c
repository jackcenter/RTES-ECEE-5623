#include <assert.h>
#include "ecclib.h"

void flip_bit(ecc_t *ecc, unsigned char *address, unsigned short bit_to_flip);

int main(void) {
  ecc_t ECC;
  unsigned char *base_addr=enable_ecc_memory(&ECC);

  // Test Case 1: NO ERRORS

  unsigned char byteToRead;

  printf("Test Case: NO ERRORS\n");
  write_byte(&ECC, base_addr, (unsigned char)0xA1);
  assert((read_byte(&ECC, base_addr, &byteToRead)) == NO_ERROR);

  // Test Case 2: pW ERROR
  printf("Test Case: pW ERROR\n");
  write_byte(&ECC, base_addr, (unsigned char)0xA2);
  flip_bit(&ECC, base_addr, 0);
  assert((read_byte(&ECC, base_addr, &byteToRead)) == PW_ERROR);

  // Test Case 3: SBE
  printf("Test Case: SBE\n");
  unsigned short bitToFlip = 10;
  write_byte(&ECC, base_addr, (unsigned char)0xA3);
  flip_bit(&ECC, base_addr, bitToFlip);
  assert((read_byte(&ECC, base_addr, &byteToRead)) == bitToFlip);

  // Test Case 4: DBE
  printf("Test Case: DBE\n");
  write_byte(&ECC, base_addr, (unsigned char)0xA4);
  flip_bit(&ECC, base_addr, 6);
  flip_bit(&ECC, base_addr, 7);
  assert((read_byte(&ECC, base_addr, &byteToRead)) == DOUBLE_BIT_ERROR);
}
