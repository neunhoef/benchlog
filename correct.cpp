#include <iostream>
#include <cmath>
#include <cstdint>
#include <string.h>

static inline uint32_t log2rough(double x) {
  uint64_t y;
  memcpy(&y, &x, 8);
  std::cout << "1: " << y << std::endl;
  std::cout << "2: " << (y >> 52) << std::endl;
  std::cout << "3: " << (uint32_t)(y >> 52) << std::endl;
  std::cout << "4: " << ((uint32_t)(y >> 52) & 0x7ff) << std::endl;
  std::cout << "5: " << ((uint32_t)(y >> 52) & 0x7ff - 1023) << std::endl;
  std::cout << "6: " << (((uint32_t)(y >> 52) & 0x7ff) - 1023) << std::endl;
  return ((uint32_t)(y >> 52) & 0x7ff) - 1023;
}

int main(int argc, char* argv[]) {
  double v = atof(argv[1]);
  std::cout << "v=" << v << std::endl;
  uint32_t r = log2rough(v);
  std::cout << "result is: " << r << std::endl;
  return 0;
}
