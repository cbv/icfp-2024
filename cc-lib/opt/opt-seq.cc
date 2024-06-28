
#include "opt-seq.h"

#include <vector>
#include <optional>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "opt/opt.h"
#include "base/logging.h"

static constexpr bool VERBOSE = false;

OptSeq::OptSeq(
    const std::vector<std::pair<double, double>> &bounds) :
  bounds(bounds) {
  if (VERBOSE) {
    printf("spawn...\n");
  }
  th.reset(new std::thread(&OptSeq::OptThread, this));
}

void OptSeq::OptThread() {

  std::vector<double> lb, ub;
  for (const auto &[l, u] : bounds) {
    lb.push_back(l);
    ub.push_back(u);
  }

  for (int offset = 0; true; offset++) {
    Opt::Minimize(
        (int)lb.size(),
        [this](const std::vector<double> &v) {
          return this->Eval(v);
        },
        lb, ub,
        1000,
        1,
        10,
        // Always use the same random seed so that we can
        // replay previous samples. But if we make more than
        // one loop, use a different seed.
        0xCAFE + offset);

    {
      std::unique_lock<std::mutex> ml(m);
      if (should_die) return;
    }
  }
}

double OptSeq::Eval(const std::vector<double> &args) {

  if (VERBOSE) {
    printf("Called:");
    for (double d : args) printf(" %.4f", d);
    printf("\n");
  }
  {
    std::unique_lock<std::mutex> ml(m);
    // When dying, just keep returning immediately to the optimizer.
    if (should_die) {
      return 0.0;
    }

    c.wait(ml, [this]() {
        return !arg.has_value();
      });

    CHECK(!arg.has_value());
    arg = {args};
  }
  c.notify_one();

  {
    std::unique_lock<std::mutex> ml(m);
    c.wait(ml, [this](){
        return result.has_value();
      });

    double r = result.value();
    // Consume the result.
    result.reset();
    return r;
  }
}

std::vector<double> OptSeq::Next() {
  std::vector<double> ret;
  {
    std::unique_lock<std::mutex> ml(m);
    CHECK(!should_die);
    c.wait(ml, [this](){
        return arg.has_value();
      });

    ret = arg.value();
    // We leave the argument, since we need this to record
    // history/best.
  }
  c.notify_one();
  return ret;
}

void OptSeq::Result(double d) {
  {
    std::unique_lock<std::mutex> ml(m);
    CHECK(!should_die);
    CHECK(arg.has_value());
    CHECK(!result.has_value());

    Observe(arg.value(), d);

    // Consumes the arg, so the next call to Next will block.
    arg.reset();
    result.emplace(d);
  }
  c.notify_one();
}

// With lock.
void OptSeq::Observe(const std::vector<double> &arg,
                     double d) {
  if (!best.has_value() ||
      best.value().second > d) {
    best.emplace(arg, d);
  }
  history.emplace_back(arg, d);
}

std::optional<std::pair<std::vector<double>, double>> OptSeq::GetBest() {
  return best;
}

OptSeq::~OptSeq() {
  {
    std::unique_lock<std::mutex> ml(m);
    should_die = true;
  }
  c.notify_all();
  th->join();
  th.reset(nullptr);
}
