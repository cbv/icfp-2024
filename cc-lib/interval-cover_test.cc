
// TODO: This test probably isn't stressing enough; there was
// a serious crasher in SplitRight (it didn't check whether next
// was .end() before trying to merge with it) that wasn't caught
// by the tests here. Consider more aggressive stress tests.

#include "interval-cover.h"

#include <set>
#include <vector>
#include <string>
#include <memory>

#include "ansi.h"
#include "arcfour.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "randutil.h"

using namespace std;

static constexpr uint64_t MAX64 = (uint64_t)-1;

template<class T>
static bool ContainsKey(const set<T> &s, const T &k) {
  return s.find(k) != s.end();
}

string SpanString(const IntervalCover<string>::Span &span) {
  string st = StringPrintf("%lld", span.start);
  string en = (span.end == MAX64) ? "MAX" :
    StringPrintf("%lld", span.end);
  return StringPrintf("[%s, \"%s\", %s)",
          st.c_str(), span.data.c_str(), en.c_str());
}


IntervalCover<string>::Span MkSpan(uint64_t start, uint64_t end,
                                   const string &data) {
  return IntervalCover<string>::Span(start, end, data);
}

static void Trivial() {
  // Check that it compiles with some basic types.
  { [[maybe_unused]] IntervalCover<int> unused(0); }
  { [[maybe_unused]] IntervalCover<double> unused(1.0); }
  { [[maybe_unused]] IntervalCover<string> unused("");  }
  { [[maybe_unused]] IntervalCover<shared_ptr<int>> unused(nullptr); }
}

static void Easy() {
  // Easy-peasy.
  IntervalCover<string> easy("everything");
  CHECK_EQ(0, easy.First());
  CHECK(!easy.IsAfterLast(0));
  CHECK(easy.IsAfterLast(MAX64));
}

// Check that the span sp is [s,e) with data d.
#define OKPOINT(s, e, d, sp) do {                             \
    const IntervalCover<string>::Span span = (sp);            \
    CHECK(span.start == (s) &&                                \
          span.end == (e) &&                                  \
          span.data == (d))                                   \
      "Wanted: " << SpanString(MkSpan((s), (e), (d)))         \
                 << "   but got   "                           \
                 << SpanString(span);                         \
  } while (0)

static void Copying() {
  IntervalCover<string> easy_a("everything");
  {
    IntervalCover<string> easy_b = easy_a;

    OKPOINT(0, MAX64, "everything", easy_a.GetPoint(0));
    OKPOINT(0, MAX64, "everything", easy_b.GetPoint(0));

    easy_a.SetSpan(10, 20, "NO");
    OKPOINT(10, 20, "NO", easy_a.GetPoint(15));
    OKPOINT(0, MAX64, "everything", easy_b.GetPoint(15));
  }

  OKPOINT(10, 20, "NO", easy_a.GetPoint(15));
}


static void DefaultCovers() {
  IntervalCover<string> simple("test");
  simple.CheckInvariants();
  for (uint64_t t : vector<uint64_t>{ 0, 80, 2, MAX64 - 1 }) {
    OKPOINT(0, MAX64, "test", simple.GetPoint(t));
  }
}

static void SimpleSplitting() {
  IntervalCover<string> simple("");
  simple.CheckInvariants();

  // Split at 1000.
  simple.SplitRight(1000, "BB");
  simple.CheckInvariants();
  OKPOINT(0, 1000, "", simple.GetPoint(999));
  OKPOINT(0, 1000, "", simple.GetPoint(0));
  OKPOINT(1000, MAX64, "BB", simple.GetPoint(1000));
  OKPOINT(1000, MAX64, "BB", simple.GetPoint(1001));

  CHECK(simple[0] == "");
  CHECK(simple[1000] == "BB");

  // Degenerate split at exactly the same point.
  simple.SplitRight(1000LL, "BB");
  simple.CheckInvariants();
  OKPOINT(0, 1000, "", simple.GetPoint(999));
  OKPOINT(0, 1000, "", simple.GetPoint(0));
  OKPOINT(1000, MAX64, "BB", simple.GetPoint(1000));
  OKPOINT(1000, MAX64, "BB", simple.GetPoint(1001));

  // Split at 1010 with new data.
  //     0------1000-----1010------
  //       ""         BB      CC
  simple.SplitRight(1010, "CC");
  simple.CheckInvariants();
  OKPOINT(0, 1000, "", simple.GetPoint(999));
  OKPOINT(1000, 1010, "BB", simple.GetPoint(1000));
  OKPOINT(1000, 1010, "BB", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));


  // Split at 10, and left interval to minimum.
  //  -----10------1000-----1010------
  //   ZZ      ""       BB        CC
  simple.SplitRight(0, "ZZ");
  simple.SplitRight(10, "");
  simple.CheckInvariants();
  OKPOINT(0, 10, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 10, "ZZ", simple.GetPoint(9));
  OKPOINT(10, 1000, "", simple.GetPoint(999));
  OKPOINT(10, 1000, "", simple.GetPoint(10));
  OKPOINT(1000, 1010, "BB", simple.GetPoint(1000));
  OKPOINT(1000, 1010, "BB", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));


  // Split at 1005.
  //  -----10------1000----1005---1010------
  //   ZZ      ""       BB      UU     CC
  simple.SplitRight(1005, "UU");
  simple.CheckInvariants();
  OKPOINT(0, 10, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 10, "ZZ", simple.GetPoint(9));
  OKPOINT(10, 1000, "", simple.GetPoint(999));
  OKPOINT(10, 1000, "", simple.GetPoint(10));
  OKPOINT(1000, 1005, "BB", simple.GetPoint(1000));
  OKPOINT(1000, 1005, "BB", simple.GetPoint(1004));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1005));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));


  // Split at 500, merging into BB
  //  -----10-----500----------1005---1010------
  //   ZZ      ""       BB          UU     CC
  simple.SplitRight(500, "BB");
  simple.CheckInvariants();
  OKPOINT(0, 10, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 10, "ZZ", simple.GetPoint(9));
  OKPOINT(10, 500, "", simple.GetPoint(10));
  OKPOINT(10, 500, "", simple.GetPoint(499));
  OKPOINT(500, 1005, "BB", simple.GetPoint(500));
  OKPOINT(500, 1005, "BB", simple.GetPoint(1004));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1005));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));

  // Split on an existing poiont (500), replacing all of BB with GG.
  //  -----10-----500----------1005---1010------
  //   ZZ      ""       GG          UU     CC
  simple.SplitRight(500, "GG");
  simple.CheckInvariants();
  OKPOINT(0, 10, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 10, "ZZ", simple.GetPoint(9));
  OKPOINT(10, 500, "", simple.GetPoint(10));
  OKPOINT(10, 500, "", simple.GetPoint(499));
  OKPOINT(500, 1005, "GG", simple.GetPoint(500));
  OKPOINT(500, 1005, "GG", simple.GetPoint(1004));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1005));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));


  // Split such that GG has to merge with the interval to its left.
  //  -----10------------------1005---1010------
  //   ZZ      ""                   UU     CC
  simple.SplitRight(500, "");
  simple.CheckInvariants();
  OKPOINT(0, 10, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 10, "ZZ", simple.GetPoint(9));
  OKPOINT(10, 1005, "", simple.GetPoint(10));
  OKPOINT(10, 1005, "", simple.GetPoint(1004));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1005));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));


  // Split to make a sandwich of UUs.
  //  -----10---20-------------1005---1010------
  //   ZZ     UU       ""          UU     CC
  simple.SplitRight(10, "temp");
  simple.SplitRight(20, "");
  simple.SplitRight(10, "UU");
  simple.CheckInvariants();
  OKPOINT(0, 10, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 10, "ZZ", simple.GetPoint(9));
  OKPOINT(10, 20, "UU", simple.GetPoint(10));
  OKPOINT(10, 20, "UU", simple.GetPoint(19));
  OKPOINT(20, 1005, "", simple.GetPoint(20));
  OKPOINT(20, 1005, "", simple.GetPoint(1004));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1005));
  OKPOINT(1005, 1010, "UU", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));

  // Now test merging on both sides.
  //  -----10-------------------------1010------
  //   ZZ              UU                  CC
  simple.SplitRight(20, "UU");
  simple.CheckInvariants();
  OKPOINT(0, 10, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 10, "ZZ", simple.GetPoint(9));
  OKPOINT(10, 1010, "UU", simple.GetPoint(10));
  OKPOINT(10, 1010, "UU", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));

  // Merge off left interval.
  //  --------------------------------1010------
  //                   ZZ                  CC
  simple.SplitRight(10, "ZZ");
  simple.CheckInvariants();
  OKPOINT(0, 1010, "ZZ", simple.GetPoint(0));
  OKPOINT(0, 1010, "ZZ", simple.GetPoint(1009));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1010));
  OKPOINT(1010, MAX64, "CC", simple.GetPoint(1011));

  // And into one interval again.
  //  --------------------------------------
  //   CC
  simple.SplitRight(0, "CC");
  simple.CheckInvariants();
  OKPOINT(0, MAX64, "CC", simple.GetPoint(0LL));
  OKPOINT(0, MAX64, "CC", simple.GetPoint(1011LL));
}

static void SplitStress() {
  ArcFour rc("splitstress");
  IntervalCover<string> cover("X");
  for (int i = 0; i < 500; i++) {
    vector<uint64_t> pts = { 0, 1, 2, 3, 4, 5, 30, 8, 10, 20,
                             999, 50, 51, 60, MAX64 - 10ULL,
                             MAX64 - 2ULL };
    Shuffle(&rc, &pts);
    int z = 0;
    for (uint64_t pt : pts) {
      z++;
      string data = StringPrintf("%llu", z);
      cover.SplitRight(pt, data);
      cover.CheckInvariants();
      CHECK_EQ(data, cover.GetPoint(pt).data);
      CHECK_LE(cover.GetPoint(pt).start, pt);
      CHECK_LT(pt, cover.GetPoint(pt).end);
    }

    // Now merge back together in random order.
    Shuffle(&rc, &pts);
    for (uint64_t pt : pts) {
      cover.SplitRight(pt, "BACK");
      cover.CheckInvariants();
      CHECK_EQ("BACK", cover.GetPoint(pt).data);
    }

    OKPOINT(0, MAX64, "BACK", cover.GetPoint(0LL));
  }
  printf("SplitStress " AGREEN("OK") "\n");
}

static void SetSpan() {
  IntervalCover<string> simple("BAD");
  simple.CheckInvariants();

  // -----------
  //      OK
  simple.SetSpan(0, MAX64, "OK");
  simple.CheckInvariants();
  OKPOINT(0, MAX64, "OK", simple.GetPoint(0LL));

  // ----- 1000 --------
  // LOW          OK
  simple.SetSpan(0, 1000LL, "LOW");
  simple.CheckInvariants();
  OKPOINT(0, 1000, "LOW", simple.GetPoint(1));
  OKPOINT(1000, MAX64, "OK", simple.GetPoint(1000));

  // ----- 900 --- 1100 --------
  // LOW      MIDDLE      OK
  simple.SetSpan(900, 1100, "MIDDLE");
  simple.CheckInvariants();

  // ----- 900 ------ 950 ---- 1150 --------
  // LOW      MIDDLE      NEXT         OK
  simple.SetSpan(950, 1150, "NEXT");
  simple.CheckInvariants();
  OKPOINT(0, 900, "LOW", simple.GetPoint(0));
  OKPOINT(900, 950, "MIDDLE", simple.GetPoint(900));
  OKPOINT(900, 950, "MIDDLE", simple.GetPoint(901));
  OKPOINT(900, 950, "MIDDLE", simple.GetPoint(949));
  OKPOINT(950, 1150, "NEXT", simple.GetPoint(950));
  OKPOINT(1150, MAX64, "OK", simple.GetPoint(1150));

  simple.SetSpan(0, MAX64, "BYE");
  simple.CheckInvariants();
  OKPOINT(0, MAX64, "BYE", simple.GetPoint(0));
  OKPOINT(0, MAX64, "BYE", simple.GetPoint(1000));
}

static void SetSpan2() {
  IntervalCover<bool> ic(false);
  auto Print = [&ic]() {
      printf("-----\n");
      for (uint64_t pt = ic.First();
           !ic.IsAfterLast(pt);
           pt = ic.Next(pt)) {
        auto span = ic.GetPoint(pt);
        printf("%llu-%llu %s\n", span.start, span.end,
               span.data ? "true" : "false");
      }
    };

  ic.SetSpan(0, 100, true);
  ic.SetSpan(100, 200, true);
  ic.SetSpan(1000, 1100, true);
  ic.SetSpan(600, 700, true);
  Print();

  CHECK(ic.GetPoint(1005).data);

  CHECK(ic.GetPoint(0).data);
  ic.SetSpan(0, 100, false);
  CHECK(!ic.GetPoint(0).data);

  CHECK(ic.GetPoint(100).data);
  ic.SetSpan(100, 200, false);
  CHECK(!ic.GetPoint(100).data);

  ic.SetSpan(0, 100000, false);
  CHECK(!ic.GetPoint(999).data);

  Print();
}

int main(int argc, char **argv) {
  ANSI::Init();

  Trivial();
  Easy();
  Copying();
  DefaultCovers();
  SimpleSplitting();
  SplitStress();
  SetSpan();
  SetSpan2();

  printf("OK\n");
  return 0;
}
