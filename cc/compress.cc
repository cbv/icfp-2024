
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>
#include <variant>
#include <string>
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

  void FactorOut(std::string_view best) {
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
  }

  static int CountHits(std::string_view needle, std::string_view haystack) {
    int hits = 0;
    for (;;) {
      auto pos = haystack.find(needle);
      if (pos == std::string_view::npos) return hits;
      hits++;
      haystack.remove_prefix(pos);
      haystack.remove_prefix(needle.size());
    }
  }

  // One pass of compression.
  bool CompressPass(int length) {
    // fprintf(stderr, "Run pass with length " AORANGE("%d") "\n", length);
    for (int part_idx = 0; part_idx < (int)parts.size(); part_idx++) {
      const Part &part = parts[part_idx];
      if (const std::string *s = std::get_if<std::string>(&part)) {
        for (size_t start = 0; start + length < s->size(); start++) {
          std::string_view sv = std::string_view(*s).substr(start, length);

          std::string_view rest = std::string_view(*s).substr(start + length,
                                                              std::string_view::npos);
          // Does it occur anywhere later?
          const int hits = [this, part_idx, sv, rest]() {
              int hits = 0;
              hits += CountHits(sv, rest);
              for (int p = part_idx + 1; p < (int)parts.size(); p++) {
                if (const std::string *s = std::get_if<std::string>(&parts[p])) {
                  hits += CountHits(sv, *s);
                }
              }
              return hits;
            }();

          // fprintf(stderr, "[%s] has %d hits\n", std::string(sv).c_str(), hits);

          if (hits > 0) {
            // Eagerly take it.
            FactorOut(sv);
            return true;
          }
        }
      }
    }

    // No hits.
    return false;
  }

  std::string VarString(int i) {
    i++;

    std::string rev;
    while (i > 0) {
      uint8_t digit = i % icfp::RADIX;
      rev.push_back(icfp::DecodeChar(digit));
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
    for (int i = 0; i < (int)named.size(); i++) {
      fprintf(stderr,
              ABLUE("%s") " " AGREY("=") " %s\n",
              VarString(i).c_str(), named[i].c_str());
    }

    // Body concatenates all the parts.
    if (parts.empty())
      return "S";

    // TODO: Join adjacent string parts first.

    std::string body = RenderPart(parts[0]);
    for (int i = 1; i < (int)parts.size(); i++) {
      body = StringPrintf("B. %s %s", body.c_str(), RenderPart(parts[i]).c_str());
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

  std::string Compress(std::string_view in) {
    const int start_size = (int)in.size();
    Periodically status_per(1.0);
    Timer timer;

    parts = {std::string(in)};

    int passes = 0;
    int max_len = in.size() / 2;

    fprintf(stderr, "START\n");

    while (max_len > MIN_LEN) {
      if (!CompressPass(max_len)) {
        max_len--;
      }

      if (status_per.ShouldRun()) {
        fprintf(stderr, "%d passes; max_len %d; %d parts; %d names\n",
                passes, max_len,
                (int)parts.size(), (int)named.size());
      }
      passes++;
    }

    std::string out = Render();
    const int out_size = (int)out.size();
    fprintf(stderr, "Done in %d passes (%s). "
            AYELLOW("%d") " -> " AGREEN("%d") "\n",
            passes, ANSI::Time(timer.Seconds()).c_str(),
            start_size, out_size);
    return out;
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
  std::string out = compressor.Compress(input_view);
  printf("%s\n", out.c_str());

  return 0;
}
