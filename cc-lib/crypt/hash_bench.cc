
#include "crypt/md5.h"
#include "crypt/sha256.h"
#include "arcfour.h"

#include <cinttypes>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>

using uint8 = uint8_t;

template<class F>
static void Bench(const std::string &name, int input_size, F f) {
  ArcFour rc("bench");

  constexpr int ITERS = 1000000;

  uint64_t sum = 0;
  const auto start = std::chrono::steady_clock::now();
  for (int iter = 0; iter < ITERS; iter++) {
	std::vector<uint8> input;
	input.reserve(input_size);
	for (int i = 0; i < input_size; i++) input.push_back(rc.Byte());

	std::vector<uint8_t> ret = f(input);
	for (uint8_t b : ret) sum += b;
  }
  const auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double> diff = end - start;
  printf("[%s] [%" PRIu64 "] Iters per second: %.1f\n",
		 name.c_str(), sum, (double)ITERS / (double)diff.count());
  fflush(stdout);
}

// Basically the minimal overhead would look something like this.
static std::vector<uint8_t> Prefix16(const std::vector<uint8_t> &input) {
  std::vector<uint8_t> pfx;
  pfx.reserve(16);
  for (int i = 0; i < 16 && i < input.size(); i++) pfx.push_back(input[i]);
  return pfx;
}

static std::vector<uint8_t> Md5(const std::vector<uint8_t> &input) {
  std::string s = MD5::Hashv(input);
  std::vector<uint8_t> ret;
  ret.resize(16);
  for (int i = 0; i < 16; i++) ret[i] = s[i];
  return ret;
}

static std::vector<uint8_t> Sha256(const std::vector<uint8_t> &input) {
  return SHA256::HashVector(input);
}
  
int main(int argc, char **argv) {
  Bench("prefix", 256, Prefix16);
  Bench("md5", 256, Md5);
  Bench("sha256", 256, Sha256);
  
  return 0;
}
