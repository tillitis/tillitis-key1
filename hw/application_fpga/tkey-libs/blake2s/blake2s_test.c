//======================================================================
//
// blake2s_test.c
// --------------
//
//======================================================================

#include <stdio.h>
#include "blake2s.h"


//------------------------------------------------------------------
//------------------------------------------------------------------
void print_message(uint8_t *m, int mlen) {
  printf("The message:\n");
  for (int i = 1 ; i <= mlen ; i++) {
    printf("0x%02x ", m[(i - 1)]);
    if (i % 8 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}


//------------------------------------------------------------------
//------------------------------------------------------------------
void print_digest(uint8_t *md) {
  printf("The digest:\n");
  for (int j = 0 ; j < 4 ; j++) {
    for (int i = 0 ; i < 8 ; i++) {
      printf("0x%02x ", md[i + 8 * j]);
    }
    printf("\n");
  }
  printf("\n");
}


//------------------------------------------------------------------
// test_zero_length()
// Test with a zero length mwssage.
//------------------------------------------------------------------
void test_zero_length() {

  uint8_t md[32];

  printf("Testing zero byte message.\n");
  blake2s(md, 32, NULL, 0, NULL, 0);
  print_digest(md);
  printf("\n");
}


//------------------------------------------------------------------
// test_abc_message()
// Test with a zero length mwssage.
//------------------------------------------------------------------
void test_abc_message() {

  uint8_t md[32];
  uint8_t msg[64] = {'a', 'b', 'c'};

  printf("Testing with RFC 7693 three byte 'abc' message.\n");
  print_message(msg, 3);

  blake2s(md, 32, NULL, 0, msg, 3);
  print_digest(md);
  printf("\n");
}


//------------------------------------------------------------------
// test_one_block_message()
// Test with a 64 byte message, filling one block.
//------------------------------------------------------------------
void test_one_block_message() {

  uint8_t md[32];
  uint8_t msg[64];

  for (uint8_t i = 0 ; i < 64 ; i++) {
    msg[i] = i;
  }

  printf("Testing with 64 byte message.\n");
  print_message(msg, 64);

  blake2s(md, 32, NULL, 0, msg, 64);
  print_digest(md);
  printf("\n");
}


//------------------------------------------------------------------
// test_one_block_one_byte_message()
// Test with a 65 byte message, filling one block and a single
// byte in the next block.
//------------------------------------------------------------------
void test_one_block_one_byte_message() {

  uint8_t md[32];
  uint8_t msg[65];

  for (uint8_t i = 0 ; i < 65 ; i++) {
    msg[i] = i;
  }

  printf("Testing with 65 byte message.\n");
  print_message(msg, 65);

  blake2s(md, 32, NULL, 0, msg, 65);
  print_digest(md);
  printf("\n");
}


//------------------------------------------------------------------
//------------------------------------------------------------------
int main(void) {
  printf("\n");
  printf("BLAKE2s reference model started. Performing a set of tests..\n");
  printf("Performing a set of tests.\n");

  test_zero_length();
  test_abc_message();
  test_one_block_message();
  test_one_block_one_byte_message();

  printf("BLAKE2s reference model completed.\n");
  printf("\n");

  return 0;
}

//======================================================================
/// EOF blake2s_test.c
//======================================================================
