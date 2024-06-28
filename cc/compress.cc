
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>
#include <variant>
#include <string>
#include <unordered_map>
#include <string_view>

#include "ansi.h"
#include "timer.h"
#include "periodically.h"
#include "base/logging.h"
#include "base/stringprintf.h"

#include "icfp.h"

// Compress a string (stdin; strips leading and trailing whitespace) into
// an ICFP expression.


// The only kind of compression this can do is factor out common substrings.
// When we find a string of the form s1Xs2X...Xsn
// we can emit
// \x. s1 ^ x ^ s2 ^ x ^ ... ^ x ^ sn

using Part = std::variant<std::string, int>;

// Each occurrence needs at least "B. vn "
static constexpr int MIN_LEN = 6;

struct Compressor {
  // strings that have been factored
  std::vector<std::string> named;

  // The current representation.
  std::vector<Part> parts;

  // One pass of compression.
  bool CompressPass() {
    // TODO: rsync tricks! This is n^2.
    std::unordered_map<std::string_view, int> seen;

    for (const Part &part : parts) {
      if (const std::string *s = std::get_if<std::string>(&part)) {
        for (size_t start = 0; start < s->size(); start++) {
          for (size_t len = MIN_LEN; start + len < s->size(); len++) {
            std::string_view sv = std::string_view(*s).substr(start, len);
            seen[sv]++;
          }
        }
      }
    }

    // Now factor out the best string. We go by the longest string
    // (with at least one repetition) in order to cut down the n^2 factors.
    std::string_view best = "";
    for (const auto &[sv, count] : seen) {
      if (count > 1) {
        if (sv.size() > best.size()) {
          best = sv;
        }
      }
    }

    if (best.empty())
      return false;

    int name = (int)named.size();
    named.push_back(std::string(best));
    std::vector<Part> new_parts;
    for (const Part &part : parts) {
      if (const int *i = std::get_if<int>(&part)) {
        new_parts.push_back(part);
      } else if (const std::string *s = std::get_if<std::string>(&part)) {
        // get any occurrences and replace them with the name

        std::string_view p(*s);
        while (!p.empty()) {
          auto pos = p.find(best);
          if (pos == std::string_view::npos) {
            new_parts.push_back(std::string(p));
            break;
          }

          std::string_view prefix = p.substr(0, pos);
          p.remove_prefix(pos);
          if (!prefix.empty()) new_parts.push_back(std::string(prefix));

          new_parts.push_back((int)name);
          p.remove_prefix(best.size());
        }
      }
    }

    parts = std::move(new_parts);
    return true;
  }

  std::string VarString(int i) {
    std::string rev;
    while (i > 0) {
      uint8_t digit = i % icfp::RADIX;
      rev.push_back(icfp::DECODE_STRING[digit]);
      i /= icfp::RADIX;
    }

    std::string s;
    s.resize(rev.size());
    for (int i = 0; i < (int)rev.size(); i++) {
      s[rev.size() - 1 - i] = rev[i];
    }
    return s;
  }

  std::string RenderPart(const Part &part) {
    if (const int *i = std::get_if<int>(&part)) {
      return StringPrintf("v%s", VarString(*i).c_str());
    } else if (const std::string *s = std::get_if<std::string>(&part)) {
      return StringPrintf("S%s", icfp::EncodeString(*s).c_str());
    } else {
      LOG(FATAL) << "Bug: Invalid Part";
    }
  }

  std::string Render() {
    // Body concatenates all the parts.
    if (parts.empty())
      return "S";

    // TODO: Join adjacent string parts first.

    std::string body = RenderPart(parts[0]);
    for (int i = 1; i < (int)parts.size(); i++) {
      body = StringPrintf("B. %s %s", RenderPart(parts[i]).c_str());
    }

    // Now binding the variables.
    auto Let = [](const std::string &v, const std::string &rhs, const std::string &bod) {
        return StringPrintf("B$ L%s %s %s",
                            v.c_str(), bod.c_str(), rhs.c_str());
      };

    for (int i = 0; i < (int)named.size(); i++) {
      body = Let(VarString(i), RenderPart(named[i]), body);
    }

    return body;
  }

  std::string Compress(const std::string &in) {
    const int start_size = (int)in.size();
    Periodically status_per(1.0);
    Timer timer;

    parts = {in};

    int passes = 0;
    for (;;) {
      bool more = CompressPass();
      if (!more) {
        std::string out = Render();
        const int out_size = (int)out.size();
        fprintf(stderr, "Done in %d passes (%s). "
                AYELLOW("%d") " -> " AGREEN("%d") "\n",
                passes, ANSI::Time(timer.Seconds()).c_str(),
                start_size, out_size);
        return out;
      }

      passes++;
    }
  }

};


int main(int argc, char **argv) {
  std::string input;
  char c;
  while (EOF != (c = fgetc(stdin))) {
    input.push_back(c);
  }

  std::string_view input_view(input);

  // Strip leading and trailing whitespace.
  while (!input_view.empty() && (input_view[0] == ' ' || input_view[0] == '\n'))
    input_view.remove_prefix(1);
  while (!input_view.empty() && (input_view.back() == ' ' || input_view.back() == '\n'))
    input_view.remove_suffix(1);

  Compressor compressor;
  std::string out = compressor.Compress(input);
  printf("%s\n", out.c_str());

  return 0;
}
