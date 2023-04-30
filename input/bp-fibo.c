#include <inttypes.h>

uint64_t read(void);
void write(uint64_t val);

int main() {
  uint64_t n = read();

  uint64_t a = 1, b = 1, tmp;
  for (int i=2; i<=n; ++i) {
      tmp = a+b;
      a = b;
      b = tmp;
  }
  write(b);

  return 0;
}
