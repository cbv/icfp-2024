
#include "autoparallel.h"

static void Simple() {
  AutoParallelComp apc(12, 10, false);

  static constexpr int SIZE = 100000;
  std::vector<int> output(SIZE, 0);

  apc.ParallelComp(
      SIZE,
      [&output](int idx) {
        output[idx] = idx;
      });

  apc.ParallelComp(
      SIZE,
      [&output](int idx) {
        output[idx]++;
      });

  for (int i = 0; i < SIZE; i++) {
    CHECK(output[i] == i + 1);
  }
}

int main(int argc, char **argv) {

  Simple();

  printf("OK\n");
}
