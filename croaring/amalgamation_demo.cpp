/* auto-generated on Tue Feb  7 06:00:34 EST 2017. Do not edit! */

#include <iostream>
#include "roaring.hh"
#include "roaring.c"
int main() {
  Roaring r1;
  for (uint32_t i = 100; i < 1000; i++) {
    r1.add(i);
  }
  std::cout << "cardinality = " << r1.cardinality() << std::endl;

  Roaring64Map r2;
  for (uint64_t i = 18000000000000000100ull; i < 18000000000000001000ull; i++) {
    r2.add(i);
  }
  std::cout << "cardinality = " << r2.cardinality() << std::endl;
  return 0;
}

