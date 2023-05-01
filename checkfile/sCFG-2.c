#include <inttypes.h>

uint64_t read(void);
void write(uint64_t val);

int main() {
  uint64_t a = read();
  if(a == 1) {
	write(a);
  }
  else if(a == 2) {
    uint64_t b = a + 1;
	write(b);
  }
  else {
    uint64_t c = a + 2;
	write(c);
  }

  write(a);
  return 0;

}
