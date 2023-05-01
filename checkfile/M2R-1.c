#include <alloca.h>

int foo(int a, int b) {
  int c = a + b;
  int* p = alloca(32);
  *p = c;
  int x = *p;
  int d = x + 10;
  return d;
}

int main() {
  return 0;
}
