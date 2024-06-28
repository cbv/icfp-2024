
#include "edit-distance.h"

#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

using namespace std;

#define score(c, d) (((c)==(d))?0:1)
#define gapscore 1

// Weirdly, this approach is fastest in benchmarks.
#define int_min(a, b) (((a)<(b))?(a):(b))
#define int_min_3(a, b, c) (((a)<(b))?int_min(a,c):int_min(b,c))

/*
inline int int_min_3(int a, int b, int c) {
  return std::min(std::min(a, b), c);
}
*/

#define ba(x, y) (a[(y) * n1 + (x)])

#define aft(i) a[(parity & (n1 + 1)) + (i)]
#define fore(i) a[((~parity) & (n1 + 1)) + (i)]

#define swaplines() parity = ~parity

int EditDistance::Distance(const string &s1, const string &s2) {
  const int n1 = s1.size();
  const int n2 = s2.size();

  int *a = (int *) malloc((n1 + 1) * 2 * sizeof(int));

  /* parity will always be either 0 or 0xFFFFFFFF */
  int parity = 0;

  // Initialize base cases.
  for (int i = 0; i <= n1; i++)
    aft(i) = gapscore * i;

  // Now, compute each row from the previous row.
  for (int y = 1; y <= n2; y ++) {
    // We know what the first column of every row is.
    int last = gapscore * y;
    fore(0) = last;

    for (int x = 1; x <= n1; x ++) {
      int xi = x - 1;
      int yi = y - 1;

      int diag = aft(x - 1) + score(s1[xi], s2[yi]);

      int up   = aft(x) + gapscore;
      int left = last + gapscore;

      last = int_min_3(up, left, diag);

      /*
      int ul = std::min(aft(x), last) + gapscore;
      last = std::min(ul, diag);
      */

      fore(x) = last;
    }

    // Move fore to aft in preparation for next line.
    swaplines();
  }

  // At this point we swapped our final line into aft, so we want
  // the last cell of aft.

  int ans = aft(n1);
  free(a);
  return ans;
}

// The following is a somewhat literal port of a JavaScript implementation
// of Ukkonen's algorithm ("Algorithms for approximate string matching")
// found at https://github.com/sunesimonsen/ukkonen
//
// Copyright (c) 2017 Sune Simonsen <sune@we-knowhow.dk>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the 'Software'), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

int EditDistance::Ukkonen(const string &s1, const string &s2, int threshold) {
  const char *a = s1.c_str();
  const char *b = s2.c_str();
  int na = s1.size();
  int nb = s2.size();
  // Put in normal form (a is the shorter one).
  if (s1.length() > s2.length()) {
    const char *t = a;
    a = b;
    b = t;
    int nt = na;
    na = nb;
    nb = nt;
  }

  while (na > 0 && nb > 0 && *a == *b) {
    a++;
    b++;
    na--;
    nb--;
  }

  if (na == 0) return std::min(nb, threshold);

  while (na > 0 && nb > 0 && a[na - 1] == b[nb - 1]) {
    na--;
    nb--;
  }

  if (na == 0) return std::min(nb, threshold);

  // Can't possibly have an edit distance larger than the longer string.
  if (nb < threshold)
    threshold = nb;

  const int nd = nb - na;

  // Cost must be at least the difference in length between the two strings,
  // so if this is more than the threshold, the answer is the threshold.
  // XXX could be <= ?
  if (threshold < nd) {
    return threshold;
  }

  // floor(min(threshold, aLen) / 2)) + 2
  const int ZERO_K = ((na < threshold ? na : threshold) >> 1) + 2;

  const int array_length = nd + ZERO_K * 2 + 2;

  int *current_row = (int *)malloc(array_length * sizeof(int));
  int *next_row = (int *)malloc(array_length * sizeof(int));

  for (int i = 0; i < array_length; i++) {
    current_row[i] = -1;
    next_row[i] = -1;
  }

  int i = 0;
  const int condition_row = nd + ZERO_K;
  const int end_max = condition_row << 1;
  do {
    i++;

    {
      int *tmp = current_row;
      current_row = next_row;
      next_row = tmp;
    }

    int start = 0;
    int next_cell = 0;

    if (i <= ZERO_K) {
      start = -i + 1;
      next_cell = i - 2;
    } else {
      start = i - (ZERO_K << 1) + 1;
      next_cell = current_row[ZERO_K + start];
    }

    int end = 0;
    if (i <= condition_row) {
      end = i;
      next_row[ZERO_K + i] = -1;
    } else {
      end = end_max - i;
    }

    int current_cell = -1;
    for (int k = start, row_index = start + ZERO_K;
         k < end;
         k++, row_index++) {
      int previous_cell = current_cell;
      current_cell = next_cell;
      next_cell = current_row[row_index + 1];

      // max(t, previous_cell, next_cell + 1)
      int t = current_cell + 1;
      t = t < previous_cell ? previous_cell : t;
      t = t < next_cell + 1 ? next_cell + 1 : t;

      while (t < na && t + k < nb && a[t] == b[t + k]) {
        t++;
      }

      next_row[row_index] = t;
    }
  } while (next_row[condition_row] < na && i <= threshold);

  free(current_row);
  free(next_row);
  return i - 1;
}

// (end copyright)

/*
        f   i   s   t   u   l   e
   *0  *1  *2  *3  *4  *5  *6  *7
 f *1  \0  \2  \3  \4  \5  \6  \7
 i *2  ^1  \0  \3  \4  \5  \6  \7
 t *3  ^2  ^1  \1  \3  \5  \6  \7
 u *4  ^3  ^2  ^2  \2  \3  \6  \7
 l *5  ^4  ^3  ^3  ^3  \3  \3  \7
 e *6  ^5  ^4  ^4  ^4  ^4  ^4  \3
*/


// Warning: I wrote this code a decade+ after the above, so
// it might follow different conventions (e.g. the matrix
// might be transposed?)
std::pair<std::vector<EditDistance::Command>, int>
EditDistance::GetAlignment(
    int n1, int n2,
    const std::function<int(int)> &deletion_cost,
    const std::function<int(int)> &insertion_cost,
    const std::function<int(int, int)> &subst_cost) {
  // Top of matrix is output; left is input.
  const int width = n2 + 1;
  const int height = n1 + 1;

  //    e    O          U        T         P       ...
  //  e 0 <- +i(O)   <- +i(U) <- +i(T)  <- +i(P)
  //  I
  //  N
  //  P
  std::vector<int> v(width * height, -1);
  auto At = [width, &v](int x, int y) -> int & {
      return v[y * width + x];
    };

  // Also record the best path. This could also be recovered
  // after the fact by recomputing the cells along the path
  // (reducing space but increasing time), but this way is less
  // fiddly.
  using uint8 = uint8_t;
  constexpr uint8 UP = 1;
  constexpr uint8 LEFT = 2;
  constexpr uint8 DIAG = 3;
  std::vector<uint8> dir(width * height, -1);
  auto DirAt = [width, &dir](int x, int y) -> uint8 & {
      return dir[y * width + x];
    };

  // Top left cell (empty to empty) is a no-op, zero cost.
  At(0, 0) = 0;

  for (int srcx = 0; srcx < n2; srcx++) {
    // First row is always insertion.
    At(srcx + 1, 0) = At(srcx, 0) + insertion_cost(srcx);
  }

  // Now fill rows.
  for (int srcy = 0; srcy < n1; srcy++) {
    const int del1 = deletion_cost(srcy);
    // First column is always deletion.
    At(0, srcy + 1) = At(0, srcy) + del1;
    for (int srcx = 0; srcx < n2; srcx++) {
      // Consider each case and take the best.
      int diag = At(srcx, srcy) + subst_cost(srcy, srcx);
      int up = At(srcx + 1, srcy) + del1;
      int left = At(srcx, srcy + 1) + insertion_cost(srcx);

      if (up < left) {
        if (diag < up) {
          At(srcx + 1, srcy + 1) = diag;
          DirAt(srcx + 1, srcy + 1) = DIAG;
        } else {
          At(srcx + 1, srcy + 1) = up;
          DirAt(srcx + 1, srcy + 1) = UP;
        }
      } else {
        if (diag < left) {
          At(srcx + 1, srcy + 1) = diag;
          DirAt(srcx + 1, srcy + 1) = DIAG;
        } else {
          At(srcx + 1, srcy + 1) = left;
          DirAt(srcx + 1, srcy + 1) = LEFT;
        }
      }
    }
  }

  #if 0
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      char c = DirAt(x, y);
      printf("%c%d  ",
             c == UP ? '^' :
             c == LEFT ? '<' :
             c == DIAG ? '\\' :
             '*', At(x, y));
    }
    printf("\n");
  }
  #endif

  // Now find the cheapest path, which gives us the actual
  // commands. cmdrev transforms (in reverse order) v1 into v2.
  std::vector<Command> cmdrev;
  cmdrev.reserve(n1 + n2);

  int x = width - 1, y = height - 1;
  const int cost = At(x, y);
  while (x > 0 || y > 0) {
    auto GoUp = [&]() {
        Command cmd;
        cmd.index1 = y - 1;
        assert(cmd.Delete());
        cmdrev.push_back(cmd);
        y--;
      };
    auto GoLeft = [&]() {
        Command cmd;
        cmd.index2 = x - 1;
        assert(cmd.Insert());
        cmdrev.push_back(cmd);
        x--;
      };

    if (x == 0) {
      // Can only move up.
      GoUp();
    } else if (y == 0) {
      GoLeft();
    } else {
      switch (DirAt(x, y)) {
      default:
        assert(!"bad dir array");
        break;
      case LEFT:
        GoLeft();
        break;
      case UP:
        GoUp();
        break;
      case DIAG: {
        Command cmd;
        cmd.index1 = y - 1;
        cmd.index2 = x - 1;
        assert(cmd.Subst());
        cmdrev.push_back(cmd);
        x--;
        y--;
        break;
      }
      }
    }
  }

  std::vector<Command> commands;
  commands.reserve(cmdrev.size());
  for (int i = cmdrev.size() - 1; i >= 0; i--)
    commands.push_back(cmdrev[i]);
  return make_pair(commands, cost);
}


