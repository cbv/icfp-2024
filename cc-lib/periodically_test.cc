
#include "periodically.h"

#include <string>
#include <optional>
#include <memory>

#include "base/logging.h"
#include "timer.h"
#include "threadutil.h"

// We could do better here (e.g. see how much total time is taken)
// but for now you need to watch to see that we get about 4 a second.
static void WatchTest() {
  Periodically pe(0.25);
  int count = 0;
  Timer run_timer;
  while (count < 10) {
    if (pe.ShouldRun()) {
      printf("%d\n", count);
      count++;
    }
  }

  double sec = run_timer.Seconds();
  printf("Took %.2f sec. Expect 2.25.\n", sec);
  // Would be more wrong for this to be too low than too high.
  CHECK(sec >= 1.75 && sec < 6.0) << sec;
}

static void ThreadedWatchTest() {
  Periodically pe(0.125);
  std::atomic<int> count{0};
  Timer run_timer;
  std::vector<std::thread> threads;
  for (int i = 0; i < 50; i++)
    threads.emplace_back([&pe, &count]() {
        while (count.load() < 20) {
          if (pe.ShouldRun()) {
            printf("%d\n", count.load());
            count++;
          }
        }
      });
  for (auto &th : threads) th.join();
  threads.clear();

  double sec = run_timer.Seconds();
  printf("Took %.2f sec. Expect ~2.25.\n", sec);
  // Would be more wrong for this to be too low than too high.
  CHECK(sec >= 1.75 && sec < 6.0) << sec;
}


int main(int argc, char **argv) {
  WatchTest();

  ThreadedWatchTest();

  printf("OK\n");
  return 0;
}
