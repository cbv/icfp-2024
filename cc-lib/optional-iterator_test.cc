
#include "optional-iterator.h"

#include <string>
#include <optional>
#include <memory>

#include "base/logging.h"

static void TestMutable() {
  {
    std::optional<std::string> empty;
    for ([[maybe_unused]] std::string &no : GetOpt(empty)) {
      CHECK(false) << "Should not have anything in here!";
    }
  }

  std::optional<std::string> yes("yes");
  int ran = 0;
  for (std::string &y : GetOpt(yes)) {
    CHECK(y == "yes");
    ran++;

    y = "cool";
  }
  CHECK(ran == 1);
  CHECK(yes.value() == "cool");
}

static void TestConst() {
  {
    const std::optional<std::string> empty;
    for ([[maybe_unused]] const std::string &no : GetOpt(empty)) {
      CHECK(false) << "Should not have anything in here!";
    }
  }

  {
    const std::optional<std::string> yes("yes");
    int ran = 0;
    for (const std::string &y : GetOpt(yes)) {
      CHECK(y == "yes");
      ran++;
    }
    CHECK(ran == 1);
    CHECK(yes.value() == "yes");
  }

  {
    // Ensure we are not inadvertently making copies.
    const std::optional<std::unique_ptr<int>> uniq(
        std::make_unique<int>(5));
    for (const std::unique_ptr<int> &ui : GetOpt(uniq)) {
      CHECK(ui != nullptr);
      CHECK(*ui == 5);
    }
  }
}

int main(int argc, char **argv) {
  TestMutable();
  TestConst();

  printf("OK\n");
  return 0;
}
