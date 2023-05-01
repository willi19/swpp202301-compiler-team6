#include <inttypes.h>

uint64_t read(void);
void write(uint64_t val);

int main() {
  uint64_t a = read();
  uint64_t b = read();

  if(a==b) {
    uint64_t c = a + b;
	uint64_t d = a * b;
	write(c);
	write(d);
  }
  
  write(a);
  return 0;
}
