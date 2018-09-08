#include <iostream>
#include "fast_rlp.h"

int main() {
  std::cout << "RUNNING\n";

  std::cout << "TEST 1\n";
  assert(FastRlpReader("\x01").item_length() == 1);

  std::cout << "TEST 2\n";
  assert(FastRlpReader("\x0f").item_length() == 1);

  std::cout << "TEST 3\n";
  assert(FastRlpReader("\x82\x04\x01").item_length() == 2);

  std::cout << "TEST 4\n";
  assert(FastRlpReader(
             "\xb8\x38Lorem ipsum dolor sit amet, consectetur adipisicing elit")
             .item_length() == 56);

  std::cout << "TEST 5\n";
  const uint8_t longstr[10003] = { 0xb9, 0x27, 0x10 };
  assert(FastRlpReader(longstr, 10003).item_length() == 10000);

  std::cout << "PASS\n";
}
