
#include "work-queue.h"

#include <string>

// Extremely basic single-threaded test. XXX Test the multi-threaded
// behavior!
static void BasicWorkQueue() {
  WorkQueue<std::string> string_queue;
  CHECK(string_queue.Size() == 0);
  string_queue.WaitAdd("hi");
  CHECK(string_queue.Size() == 1);

  std::optional<std::string> oi = string_queue.WaitGet();
  CHECK(oi.has_value() && oi.value() == "hi");
  CHECK(string_queue.Size() == 0);

  string_queue.MarkDone();
  std::optional<std::string> oi2 = string_queue.WaitGet();
  CHECK(!oi2.has_value());
}

int main(int argc, char **argv) {

  BasicWorkQueue();


  printf("OK\n");
  return 0;
}
