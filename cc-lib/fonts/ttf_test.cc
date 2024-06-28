
#include "ttf.h"

#include <cstdio>

// XXX some more tests!

static void CreateAndDestroy() {
  TTF ttf("DFXPasement9px.ttf");
}

int main(int argc, char **argv) {
  CreateAndDestroy();

  printf("OK\n");
  return 0;
}
