#include "fast_rlp.h"
#include <iostream>

int main() {
  std::cout << "RUNNING\n";
  assert(FastRlpReader("\x00").item_length() == 1);
  assert(FastRlpReader("\x0f").item_length() == 1);
  assert(FastRlpReader("\x82\x04\x00").item_length() == 2);
  assert(FastRlpReader(
             "\xb8\x38Lorem ipsum dolor sit amet, consectetur adipisicing elit")
             .item_length() == 56);

  const uint8_t longstr[10000] = { 0xb9, 0x27, 0x10 };
  assert(FastRlpReader(longstr, 10002).item_length() == 10000);

  std::cout << "PASS\n";
}
