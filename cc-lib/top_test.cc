
#include <vector>

#include "top.h"

using namespace std;

int main(int argc, char **argv) {

  // Note: Can't fail! Perhaps could search for argv[0]?
  for (const string &proc : Top::Enumerate()) {
    printf("%s\n", proc.c_str());
  }

  return 0;
}
