
#include "boxes-and-glue.h"

#include <memory>
#include <cmath>
#include <vector>
#include <utility>
#include <unordered_map>
#include <cstdio>
#include <unordered_set>

#include "base/logging.h"
#include "ansi.h"
#include "hashing.h"

static constexpr bool VERBOSE = false;

using Justification = BoxesAndGlue::Justification;

enum class LineJustification {
  CENTER,
  LEFT,
  JUSTIFY,
  // RIGHT,
};

static LineJustification NormalLineJustification(Justification just) {
  switch (just) {
  case Justification::FULL: return LineJustification::JUSTIFY;
  case Justification::ALL: return LineJustification::JUSTIFY;
  case Justification::CENTER: return LineJustification::CENTER;
  case Justification::LEFT: return LineJustification::LEFT;
  }
  LOG(FATAL) << "Unimplemented justification";
  return LineJustification::LEFT;
}

static LineJustification FinalLineJustification(Justification just) {
  switch (just) {
  case Justification::FULL: return LineJustification::LEFT;
  case Justification::ALL: return LineJustification::JUSTIFY;
  case Justification::CENTER: return LineJustification::CENTER;
  case Justification::LEFT: return LineJustification::LEFT;
  }
  LOG(FATAL) << "Unimplemented justification";
  return LineJustification::LEFT;
}

// Old, greedy version. Maybe useful for comparison in the paper?
// Probably can move to BoxesAndGlue
std::vector<std::vector<BoxesAndGlue::BoxOut>>
BoxesAndGlue::PackBoxesFirst(
    double line_width,
    const std::vector<BoxIn> &boxes_in,
    double max_break_penalty,
    Justification just) {
  static constexpr bool VERBOSE = false;

  for (int i = 0; i < (int)boxes_in.size(); i++) {
    CHECK(boxes_in[i].parent_idx == i - 1 &&
          boxes_in[i].edge_penalty == 0.0) << "PackBoxesFirst does "
      "not support tree input";
  }

  using BoxOut = BoxesAndGlue::BoxOut;

  std::vector<std::vector<BoxesAndGlue::BoxOut>> lines_out;

  std::vector<BoxesAndGlue::BoxOut> current_line;
  auto EmitLine = [&](LineJustification line_just) {
      if (current_line.empty()) return;

      // How much left-over space is there?
      double used = 0.0;
      for (const BoxesAndGlue::BoxOut &out : current_line) {
        used += out.box->width;
        used += out.actual_glue;
      }

      double leftover = line_width - used;

      switch (line_just) {
      case LineJustification::CENTER:
        // Symmetric outer space.
        current_line[0].left_padding += leftover * 0.5;
        current_line.back().actual_glue += leftover * 0.5;
        break;

      case LineJustification::LEFT:
        // This is typically invisible, but we might
        // as well.
        current_line.back().actual_glue += leftover;
        break;

      case LineJustification::JUSTIFY: {

        double total_weight = 0.0;
        for (int i = 0; i < (int)current_line.size() - 1; i++) {
          double weight =
            leftover >= 0.0 ?
            current_line[i].box->glue_expand :
            current_line[i].box->glue_contract;
          total_weight += weight;
        }

        double weighted_glue = leftover / total_weight;

        for (int i = 0; i < (int)current_line.size() - 1; i++) {
          double weight =
            leftover >= 0.0 ?
            current_line[i].box->glue_expand :
            current_line[i].box->glue_contract;
          current_line[i].actual_glue += weighted_glue * weight;
        }

        break;
      }
      }

      lines_out.push_back(std::move(current_line));
      current_line.clear();
    };

  double current_width = 0.0;
  double current_postwidth = 0.0;
  bool cannot_break = false;
  for (const BoxIn &box : boxes_in) {

    BoxOut boxo;
    boxo.box = &box;

    const bool fits =
      current_width + current_postwidth + box.width <= line_width;
    if (cannot_break || fits) {
      // Take the box.
      if (VERBOSE) {
        printf(AGREEN("take") "%s%s\n\n",
               cannot_break ? " (cannot-break)" : "",
               fits ? " (fits)" : "");
      }

      // This means the previous box gets its glue turned into padding.
      if (!current_line.empty()) {
        current_width += current_postwidth;
        current_line.back().actual_glue = current_postwidth;
        current_line.back().penalty_here +=
          current_line.back().box->glue_break_penalty;
      } else {
        CHECK(current_postwidth == 0.0);
      }

      current_line.push_back(boxo);
      current_width += box.width;
      cannot_break = box.glue_break_penalty > max_break_penalty;
      current_postwidth = box.glue_ideal;

    } else {
      // Break.
      if (VERBOSE) {
        printf(AORANGE("break") "\n\n");
      }

      if (current_line.empty()) {
        // Unusual situation where the word was so long (or break was
        // not allowed) that it doesn't fit on a line on its own. We
        // put it on the line, but do not "break" before it.

        // XXX there should be badness if the word overruns.
      } else {
        current_line.back().did_break = true;
        current_line.back().penalty_here =
          current_line.back().box->glue_break_penalty +
          std::abs(line_width - current_width);
      }

      // This does not output empty lines, so it works for the unusual
      // case above, too.
      EmitLine(NormalLineJustification(just));

      current_line = {boxo};
      current_width = box.width;
      cannot_break = box.glue_break_penalty > max_break_penalty;
      current_postwidth = box.glue_ideal;
    }
  }

  // The line usually still has something on it.
  EmitLine(FinalLineJustification(just));
  return lines_out;
}

namespace {
struct MemoResult {
  double penalty;
  int successor;
  bool break_after;
};

using MemoTable = std::unordered_map<std::pair<int, int>, MemoResult,
                                     Hashing<std::pair<int, int>>>;

struct TableImpl : public BoxesAndGlue::Table {
  ~TableImpl() override {}
  TableImpl(int w, int h, MemoTable table) :
    width(w), height(h), memo_table(std::move(table)) {

  }

  int Width() const override { return width; }
  int Height() const override { return height; }

  std::optional<std::tuple<double, int, bool>>
  GetCell(int x, int y) const override {
    auto it = memo_table.find(std::make_pair(x, y));
    if (it == memo_table.end()) return std::nullopt;
    else return std::make_optional(std::make_tuple(it->second.penalty,
                                                   it->second.successor,
                                                   it->second.break_after));
  }

  int width = 0, height = 0;
  MemoTable memo_table;
};
}  // namespace

BoxesAndGlue::Table::~Table() {}

std::vector<std::vector<BoxesAndGlue::BoxOut>> BoxesAndGlue::PackBoxes(
    double line_width,
    const std::vector<BoxIn> &boxes,
    Justification just,
    std::unique_ptr<Table> *table) {

  std::vector<std::vector<std::pair<int, double>>> successors(
      boxes.size(), std::vector<std::pair<int, double>>{});

  std::vector<int> starting_nodes;
  std::vector<int> depth;
  depth.reserve(boxes.size());

  for (int i = 0; i < (int)boxes.size(); i++) {
    const BoxIn &box = boxes[i];
    CHECK(box.parent_idx < i) << "The nodes must be "
      "topologically sorted, for one thing to inhibit cycles.";
    CHECK(box.parent_idx == -1 ||
          (box.parent_idx >= 0 &&
           box.parent_idx < (int)boxes.size()));
    if (box.parent_idx == -1) {
      starting_nodes.push_back(i);
      depth.push_back(0);
    } else {
      CHECK(box.parent_idx >= 0) << box.parent_idx;
      successors[box.parent_idx].emplace_back(i, box.edge_penalty);
      CHECK(box.parent_idx < (int)depth.size());
      depth.push_back(depth[box.parent_idx] + 1);
    }
  }

  std::unordered_set<int> ending_nodes;
  for (int i = 0; i < (int)boxes.size(); i++) {
    if (successors[i].empty()) ending_nodes.insert(i);
  }

  // Once a line is complete, apply glue. The line justification
  // determines how we apply the glue.
  auto ApplyGlue = [line_width](std::vector<BoxOut> *current_line,
                                LineJustification justify) {

      double space_used = 0.0;
      for (int i = 0; i < (int)current_line->size(); i++) {
        const BoxOut &box = (*current_line)[i];
        space_used += box.box->width;
        if (i < (int)current_line->size() - 1) {
          space_used += box.box->glue_ideal;
        } else {
          CHECK(i == (int)current_line->size() - 1);
          // Last word behaves differently. It has no glue
          // and might have its width adjusted for a hyphen.
          space_used += box.box->glue_break_extra_width;
        }
      }

      // could be negative
      const double space_remaining = line_width - space_used;
      const bool expanding = space_remaining >= 0.0;

      double total_weight = 0.0;
      for (int i = 0; i < (int)current_line->size() - 1; i++) {
        BoxOut &box = (*current_line)[i];
        total_weight += expanding ? box.box->glue_expand :
          box.box->glue_contract;
      }

      const double weighted_glue = space_remaining / total_weight;

      if (VERBOSE) {
        printf("Total weight: %.11g.\n"
               "Weighted glue: %.11g\n", total_weight, weighted_glue);
      }

      for (int i = 0; i < (int)current_line->size() - 1; i++) {
        BoxOut &box = (*current_line)[i];
        const double weight = expanding ? box.box->glue_expand :
          box.box->glue_contract;

        box.actual_glue = box.box->glue_ideal;
        // How much glue to add between words?
        if (justify == LineJustification::JUSTIFY) {
          const double additional_glue = weighted_glue * weight;
          if (VERBOSE) {
            printf("  Additional glue: %.11g\n", additional_glue);
          }
          box.actual_glue += additional_glue;
        }
      }

      if (justify == LineJustification::CENTER && !current_line->empty()) {
        // Not using weighted glue.
        double center_space = space_remaining * 0.5;
        (*current_line)[0].left_padding = center_space;
        // printf("Added center space of %.3f\n", center_space);
        current_line->back().actual_glue += center_space;
      }

    };

  // This is a dynamic programming problem. We store a table of O(m^2)
  // entries. The table is keyed by a pair: A word index and the
  // number of words before that word on the line. The number of words
  // also uniquely determines which words they are (the ones right
  // before the indicated word!) and thus the starting position of
  // this word on the line, wherever we are.
  //
  // The value is the result of computing the best way of laying out
  // this word and the remainder, given this situation. The MemoResult
  // pair contains the total penalty (for the rest), the successor node
  // to choose, and whether this best layout breaks after this word.
  std::unordered_map<std::pair<int, int>, MemoResult,
    Hashing<std::pair<int, int>>> memo_table;

  // Same arguments as the memo table. Gets the width of the text up
  // to word_idx on this line. This assumes that each word has ideal
  // glue applied; that is clear with respect to glue_break_extra_width
  // (since there are no breaks). For expanding/contracting glue, we
  // do this proportionally using the leftover space once we know
  // where the line ends.
  //
  // words_before cannot be more than the depth.
  auto GetWidthBefore = [&](int word_idx, int words_before) -> double {
      CHECK(words_before <= depth[word_idx]);

      double width_used = 0.0;
      for (int b = 0; b < words_before; b++) {
        // get previous word
        CHECK(word_idx >= 0);
        word_idx = boxes[word_idx].parent_idx;

        const BoxIn &box = boxes[word_idx];
        CHECK(word_idx >= 0);
        width_used += box.width + box.glue_ideal;
      }

      return width_used;
    };

  // Since the recursion depth can get kinda high here, we need to solve
  // this one iteratively. It's bottom-up, starting from the leaves.
  // Since the nodes are topologically sorted, we'll always have the
  // successor nodes filled in.
  for (int word_idx = (int)boxes.size() - 1; word_idx >= 0; word_idx--) {
    // As a loop invariant, we have memo_table filled in for every greater
    // word_idx.

    // Look up the entry in the memo table, handling the base cases
    // beyond the box vector.
    auto Get = [&boxes, &memo_table](int w, int b) -> MemoResult {
        // Base case is no penalty; no breaks.
        if (w >= (int)boxes.size())
          return MemoResult{
            .penalty = 0.0,
            .successor = -1,
            .break_after = false
          };
        auto mit = memo_table.find(std::make_pair(w, b));
        CHECK(mit != memo_table.end()) << "Later table entries should "
          "be filled in!" << w << "," << b;
        return mit->second;
      };

    // Set the entry in the memo table.
    auto Set = [&boxes, &memo_table](int w, int b, MemoResult mr) {
        CHECK(!memo_table.contains(std::make_pair(w, b))) <<
          "Duplicate entries?";
        if (VERBOSE) {
          if (w < (int)boxes.size()) {
            printf(
                "  Penalty ..%d.. ["
                AWHITE("box #%d") "] = " ARED("%.4f") " -> #%d %s\n",
                b, w, mr.penalty, mr.successor,
                mr.break_after ? AYELLOW("break") : "no");
          }
        }
        memo_table[std::make_pair(w, b)] = mr;
      };

    // Now compute the value in the table for every number of words before
    // the first one, back to the beginning of the input.
    //
    // PERF: We can (and should) cut this off once we exceed the
    // length of a line.
    for (int words_before = 0;
         words_before <= depth[word_idx];
         words_before++) {

      #if 0
      if (VERBOSE) {
        printf("[%d,%d] Check", word_idx, words_before);
        const int before_start = word_idx - words_before;
        CHECK(before_start >= 0);
        for (int b = 0; b < words_before; b++) {
          // Add the word's length and the space after it.
          CHECK(before_start + b < (int)boxes.size());
          printf(" " AGREY("box #%d"), before_start + b);
        }
        printf(" " AWHITE("box #%d") "\n", word_idx);
      }
      #endif

      // PERF: Can compute this incrementally in the loop.
      CHECK(word_idx >= 0 && word_idx < (int)boxes.size()) << word_idx;
      // This includes trailing spaces.
      const double width_before = GetWidthBefore(word_idx, words_before);
      // And always add the word.
      CHECK(word_idx < (int)boxes.size()) << word_idx
                                          << " vs " << boxes.size();
      const BoxIn &box = boxes[word_idx];

      const double width_word_nobreak = box.width;
      const double width_word_break = box.width + box.glue_break_extra_width;

      double penalty_word_nobreak = 0.0;
      double penalty_word_break = 0.0;

      const double total_width_nobreak = width_before + width_word_nobreak;
      const double total_width_break = width_before + width_word_break;

      if (VERBOSE) {
        printf("  %.4f > %.4f? ",
               total_width_nobreak, line_width);
      }
      if (total_width_nobreak > line_width) {
        if (VERBOSE) {
          printf(ABGCOLOR(255,0,0, "OVER"));
        }
        if (width_before > line_width) {
          // We were already over. So just add the word's size.
          // This might be wrong wrt the trailing space, although
          // in this case these details just amount to tweaks to the
          // multiplier.
          penalty_word_nobreak += width_word_nobreak;
          if (VERBOSE) printf(" full penalty %.3f", width_word_nobreak);
        } else {
          // Since this is the word that puts us over, only count
          // the amount that it's over.
          penalty_word_nobreak += (total_width_nobreak - line_width);
          if (VERBOSE) printf(" part penalty %.3f", penalty_word_nobreak);
        }
      }
      // But the overage penalty is scaled.
      if (penalty_word_nobreak > 0.0) {
        penalty_word_nobreak = (1.0 + penalty_word_nobreak);
        penalty_word_nobreak = penalty_word_nobreak *
          penalty_word_nobreak * penalty_word_nobreak;
      }
      if (VERBOSE) {
        printf("\n");
      }

      // Also the same thing for a break, since we may have a different
      // width in that case (e.g. because of an inserted hyphen).
      if (total_width_break > line_width) {
        if (width_before > line_width) {
          penalty_word_break += width_word_break;
        } else {
          penalty_word_break += (total_width_break - line_width);
        }
      }
      if (penalty_word_break > 0.0) {
        penalty_word_break = (1.0 + penalty_word_break);
        penalty_word_break = penalty_word_break *
          penalty_word_break * penalty_word_break;
      }

      // The penalties above just depend on this box and before_words.
      // Now compute the best option, checking each possible
      // continuation.

      if (successors[word_idx].empty()) {
        MemoResult only;
        only.penalty = penalty_word_nobreak;
        only.successor = -1;
        only.break_after = false;
        Set(word_idx, words_before, only);
      } else {
        MemoResult best;
        best.penalty = 1.0/0.0;
        best.successor = -999;
        best.break_after = false;

        for (const auto &[next_node, edge_penalty] : successors[word_idx]) {
          // For each of these, we can either break here, or continue.

          // If we break, then the penalty is the amount of space left.
          const double penalty_break_slack_base =
            std::max(line_width - total_width_break, 0.0);

          // We want to use a nonlinear scale for slack. Otherwise it
          // wouldn't matter where we put it, and this can result in
          // putting it all on the same line (which clearly looks worse).
          const double penalty_break_slack =
            std::pow(penalty_break_slack_base, 1.8);

          // ... plus the penalty for the remainder, starting on a new line.
          const double p_rest = Get(next_node, 0).penalty;

          const double penalty_break =
            // Some glue comes with a penalty, e.g. because we have
            // to insert a hyphen. (But the penalty can also be
            // negative!)
            box.glue_break_penalty +
            // Just following the edge costs this amount.
            edge_penalty +
            penalty_word_break + penalty_break_slack + p_rest;

          if (VERBOSE) {
            printf("for successor %d:\n"
                   "  width used " ABLUE("%.4f") "."
                   "Word penalty " APURPLE("%.4f") ".\n"
                   "    w/break " AORANGE("%.4f")
                   " " AGREY("(slack)") " + " AYELLOW("%.4f")
                   " " AGREY("(rest)") " = " ARED("%.4f") "\n",
                   next_node,
                   total_width_break, penalty_word_break,
                   penalty_break_slack, p_rest, penalty_break);
          }

          // And the case where we do not break.
          const double p_rest_nobreak =
            Get(next_node, words_before + 1).penalty;
          const double penalty_nobreak =
            // Just following the edge costs this amount.
            edge_penalty +
            penalty_word_nobreak + p_rest_nobreak;
          if (VERBOSE) {
            printf("    or without break: "
                   AGREEN("%.4f") " = " ARED("%.4f") "\n",
                   p_rest_nobreak, penalty_nobreak);
          }

          // Consider both options.
          if (penalty_break < best.penalty) {
            best = MemoResult{
              .penalty = penalty_break,
              .successor = next_node,
              .break_after = true,
            };
          }

          if (penalty_nobreak < best.penalty) {
            best = MemoResult{
              .penalty = penalty_nobreak,
              .successor = next_node,
              .break_after = false,
            };
          }
        }

        CHECK(best.successor >= 0);
        // Now save the best one.
        Set(word_idx, words_before, best);
      }
    }
  }


  // Now lay it out using the memo table we already computed.
  std::vector<std::vector<BoxOut>> lines;
  int before = 0;
  std::vector<BoxOut> current_line;

  // Get best starting node.
  int start = -1;
  double start_penalty = 1.0/0.0;
  for (int w : starting_nodes) {
    const auto mit = memo_table.find(std::make_pair(w, 0));
    CHECK(mit != memo_table.end()) << "Bug: We should have computed the "
      "value for each starting node above! " << w;
    if (start == -1 || mit->second.penalty < start_penalty) {
      start = w;
      start_penalty = mit->second.penalty;
    }
  }

  int w = start;
  while (w != -1) {
    // Get the data from the table.
    const auto mit = memo_table.find(std::make_pair(w, before));
    CHECK(mit != memo_table.end()) << "Bug: This should have been computed by "
      "the procedure above; we're retracing its steps here: " <<
      w << "," << before;

    CHECK((int)current_line.size() == before) << "Table inconsistency: " <<
      current_line.size() << " vs " << before;

    // Now, do we break or not?
    MemoResult memo_result = mit->second;
    if (VERBOSE) {
      printf("After [" AWHITE("box #%d") "]? -> #%d, "
             "Penalty " ARED("%.4f") " %s\n",
             w, memo_result.successor, memo_result.penalty,
             memo_result.break_after ? AYELLOW("break") : AGREY("no"));
    }

    BoxOut box_out;
    box_out.box = &boxes[w];
    box_out.box_idx = w;
    box_out.did_break = memo_result.break_after;
    // Note: We diff these down below.
    box_out.penalty_here = memo_result.penalty;
    current_line.push_back(box_out);

    if (memo_result.break_after) {
      LineJustification lj = LineJustification::LEFT;
      switch (just) {
      case Justification::FULL:
      case Justification::ALL: lj = LineJustification::JUSTIFY; break;
      case Justification::CENTER: lj = LineJustification::CENTER; break;
      case Justification::LEFT: lj = LineJustification::LEFT; break;
      }

      ApplyGlue(&current_line, lj);
      lines.push_back(std::move(current_line));
      current_line.clear();
      before = 0;
    } else {
      before++;
    }

    w = memo_result.successor;
  }

  if (!current_line.empty()) {
    // Apply glue to final line. It is not justified unless
    // we are in "all" mode.

    ApplyGlue(&current_line, FinalLineJustification(just));
    lines.push_back(std::move(current_line));
  }

  // Compute penalty diffs.
  BoxOut *prev = nullptr;
  for (auto &line : lines) {
    for (auto &box : line) {
      if (prev != nullptr) {
        prev->penalty_here -= box.penalty_here;
      }
      prev = &box;
    }
  }

  if (table != nullptr) {
    table->reset(new TableImpl((int)boxes.size(),
                               (int)boxes.size(),
                               std::move(memo_table)));
  }

  return lines;
}

