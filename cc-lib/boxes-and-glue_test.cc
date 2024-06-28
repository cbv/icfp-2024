
#include "boxes-and-glue.h"

#include <string>
#include <vector>

#include "util.h"
#include "ansi.h"
#include "color-util.h"

using BoxIn = BoxesAndGlue::BoxIn;
using BoxOut = BoxesAndGlue::BoxOut;
using Justification = BoxesAndGlue::Justification;

static void TestASCII(bool best_fit, Justification just) {
  std::vector<std::string> words =
    Util::Split(
        "If during a game it is found that an illegal move, including "
        "failing to meet the requirements of the promotion of a pawn or "
        "capturing the opponent's king, een completed, the position "
        "immediately before the irregularity shall be reinstated. If the "
        "position immediately before the irregularity cannot be "
        "determined the game shall continue from the last identifiable "
        "position prior to the irregularity. The clocks shall be adjusted "
        "according to Article 6.13. The Articles 4.3 and 4.6 apply to the "
        "move replacing the illegal move. The game shall then continue "
        "from this reinstated position.", ' ');

  std::vector<BoxIn> boxes;
  for (int i = 0; i < (int)words.size(); i++) {
    BoxIn box;
    box.width = words[i].size();
    box.glue_break_penalty = 0.0;
    box.glue_break_extra_width = 0.0;
    box.glue_ideal = 1.0;
    box.glue_expand = 1.0;
    box.glue_contract = 1.0;
    box.parent_idx = i - 1;
    box.edge_penalty = 0.0;
    box.data = (void*)&words[i];
    boxes.push_back(box);
  }

  std::vector<std::vector<BoxOut>> out =
    best_fit ? BoxesAndGlue::PackBoxes(40.0, boxes, just) :
    BoxesAndGlue::PackBoxesFirst(40.0, boxes, 5.0);

  double max_penalty = 0.0;
  for (const std::vector<BoxOut> &line : out) {
    for (const BoxOut &box : line) {
      max_penalty = std::max(std::abs(box.penalty_here), max_penalty);
    }
  }

  for (const std::vector<BoxOut> &line : out) {
    // Alas, we can only space in discrete increments.
    double slack = 0.0;
    for (const BoxOut &word : line) {

      slack += word.left_padding;
      while (slack >= 1.0) {
        printf(" ");
        slack -= 1.0;
      }

      const std::string &w = *(const std::string*)word.box->data;
      slack += word.actual_glue;

      uint32_t color = ColorUtil::LinearGradient32(
          ColorUtil::HEATED_TEXT, word.penalty_here / max_penalty);

      printf("%s%s" ANSI_RESET,
             ANSI::ForegroundRGB32(color).c_str(),
             w.c_str());
      while (slack >= 1.0) {
        printf(" ");
        slack -= 1.0;
      }
    }
    printf("\n");
  }
}

int main(int argc, char **argv) {
  ANSI::Init();

  printf("----------- first ---------\n");
  TestASCII(false, Justification::FULL);
  printf("----------- best ----------\n");
  TestASCII(true, Justification::FULL);
  printf("----------- left ----------\n");
  TestASCII(true, Justification::LEFT);
  printf("----------- center --------\n");
  TestASCII(true, Justification::CENTER);

  printf("OK\n");
  return 0;
}
