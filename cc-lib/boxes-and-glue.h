
#ifndef _CC_LIB_BOXES_AND_GLUE
#define _CC_LIB_BOXES_AND_GLUE

#include <memory>
#include <vector>
#include <tuple>
#include <optional>

// Abstract version of a boxes-and-glue algorithm.
//
// This is presented as the classic problem of placing words into
// lines with optinal line breaks. But it can be used for other stuff,
// like placing paragraphs into columns. Note that even when placing
// words on a line, we often have more than one box per word, to
// represent potential hyphenation points or kerning pairs.
//
// Note that this is probably not the same algorithm as Knuth's.
struct BoxesAndGlue {

  enum class Justification {
    // Lines take the full width, except for the last.
    FULL,
    LEFT,
    CENTER,
    // Lines take the full width, including the last.
    ALL,
  };

  struct BoxIn {
    // Native width of the box, not counting any glue.
    double width = 0.0;
    // Additional penalty if we break here.
    double glue_break_penalty = 0.0;
    // If we break here, then this much extra width is used
    // (for insertion of a hyphen or something like that).
    double glue_break_extra_width = 0.0;
    // Ideal amount of glue after the box, e.g. the width
    // of a space glyph in the font (or a kerning pair).
    double glue_ideal = 0.0;

    // Coefficients used when applying glue in a line.
    // When a coefficient is higher, larger magnitude of space
    // (positive or negative) is apportioned here.
    double glue_expand = 1.0;
    double glue_contract = 1.0;

    // Required! Must be less than the box's index.
    // If parent_node is -1, then it is a root.
    // A node that doesn't have any children is terminal.
    // In the normal case of laying out a flat vector of words,
    // just set the parent to index-1 for each node.
    int parent_idx = 0;
    double edge_penalty = 0.0;

    // User data.
    void *data = nullptr;
  };

  struct BoxOut {
    // Points to one of the boxes in the input vector.
    const BoxIn *box = nullptr;
    // Refers to that same box by its index.
    int box_idx = 0;

    // Populated by PackBoxes.
    // Did we break after this box?
    bool did_break = false;
    // Amount of glue assigned (on the right side). This does not
    // include width or glue_break_extra_width.
    double actual_glue = 0.0;

    // This is only used on the first box in a line to add left
    // padding when rendering CENTERED or RIGHT. Note this can be
    // negative when the line is too long.
    double left_padding = 0.0;

    // Penalty associated with this word. Penalties are a global
    // phenomenon, so don't read too much into it being associated
    // with this word (it is almost always the last word in the line).
    // It's just the difference betwen the penalty at this spot versus
    // the tail. Summing up all the penalties in the output gives the
    // total badness.
    double penalty_here = 0.0;
  };

  struct Table {
    // The x dimension is the word index.
    virtual int Width() const = 0;
    // The y dimension is the number of words before this one
    // on the line.
    virtual int Height() const = 0;

    // penalty, successor, break?
    virtual std::optional<std::tuple<double, int, bool>>
    GetCell(int x, int y) const = 0;
    virtual ~Table();
  };

  // Return the vector of "lines," each with the vector of "words"
  // on that line.
  static std::vector<std::vector<BoxOut>> PackBoxes(
      double line_width,
      const std::vector<BoxIn> &boxes_in,
      Justification justification = Justification::FULL,
      // For debugging / visualization, the memo table.
      std::unique_ptr<Table> *table = nullptr);

  // Greedy algorithm, mostly useful for comparison purposes or
  // as a fallback (it's linear time).
  // Doesn't support tree input!
  static std::vector<std::vector<BoxOut>> PackBoxesFirst(
      double line_width,
      const std::vector<BoxIn> &boxes_in,
      double max_break_penalty,
      Justification justification = Justification::LEFT);

};

#endif
