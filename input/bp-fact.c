#include <inttypes.h>

uint64_t read(void);
void write(uint64_t val);

uint64_t fact(int n) {
    if (n == 0) return 1;
    return fact(n-1) * n;
}

int main() {
  uint64_t n = read();
  
  write(fact(n));
  
  return 0;
}
