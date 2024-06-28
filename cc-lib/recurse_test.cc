
#include "recurse.h"

#include <functional>
#include <cstdio>

#include "base/logging.h"

// template<class Self>
int Fib(const std::function<int(int)> &self, int x) {
  if (x <= 0) return 0;
  if (x == 1) return 1;
  return self(x - 1) + self(x - 2);
}

static void Fibonacci() {
  auto Rec = Recursive<int, int>(Fib);

  int r = Rec(8);
  CHECK(r == 21);
}


int main(int argc, char **argv) {
  Fibonacci();
  printf("OK\n");
}


