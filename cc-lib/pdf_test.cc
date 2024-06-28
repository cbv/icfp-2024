
#include "pdf.h"

#include <cmath>
#include <numbers>
#include <string>
#include <vector>
#include <utility>

#include "base/logging.h"
#include "ansi.h"
#include "image.h"
#include "arcfour.h"
#include "util.h"

static constexpr std::initializer_list<const char *> FONTS = {
  "cmunrm.ttf",
  // "c:\\windows\\fonts\\pala.ttf",
  "cour.ttf",
};

using SpacedLine = PDF::SpacedLine;
using FontObj = PDF::FontObj;

static std::vector<std::pair<float, float>> Star(
    float x, float y,
    float r1, float r2,
    int tines) {

  std::vector<std::pair<float, float>> ret;
  ret.reserve(tines * 2);
  for (int i = 0; i < tines; i++) {
    double theta1 = ((i * 2 + 0) / double(tines * 2)) *
      2.0 * std::numbers::pi;
    double theta2 = ((i * 2 + 1) / double(tines * 2)) *
      2.0 * std::numbers::pi;
    ret.emplace_back(x + cosf(theta1) * r1, y + sinf(theta1) * r1);
    ret.emplace_back(x + cosf(theta2) * r2, y + sinf(theta2) * r2);
  }
  return ret;
}

static ImageRGB RandomRGB(ArcFour *rc, int width, int height) {
  ImageRGB img(width, height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t r = rc->Byte();
      uint8_t g = rc->Byte();
      uint8_t b = rc->Byte();
      img.SetPixel(x, y, r, g, b);
    }
  }
  return img;
}

static void SpaceLine() {
  PDF pdf(PDF::PDF_LETTER_WIDTH, PDF::PDF_LETTER_HEIGHT);
  PDF::Info info;
  sprintf(info.creator, "pdf_test.cc");
  sprintf(info.producer, "Tom 7");
  sprintf(info.title, "It is a test");
  sprintf(info.author, "None");
  sprintf(info.author, "No subject");
  sprintf(info.date, "30 Dec 2023");
  pdf.SetInfo(info);

  const FontObj *times = pdf.GetBuiltInFont(PDF::TIMES_ROMAN);

  {
    std::vector<PDF::SpacedLine> lines =
      pdf.SpaceLines("simple", 1000, times);
    CHECK(lines.size() == 1);
    const SpacedLine &line = lines[0];
    CHECK(line.size() == 1);
    CHECK(line[0].first == "simple");
  }

  {
    std::vector<PDF::SpacedLine> lines =
      pdf.SpaceLines("simple one", 1000, times);
    CHECK(lines.size() == 1);
    const SpacedLine &line = lines[0];
    CHECK(line.size() == 3);
    CHECK(line[0].first == "simple");
    CHECK(line[1].first == " ");
    CHECK(line[2].first == "one");
  }

  {
    double width = 0.9 * times->GetKernedWidth("simple one");
    std::vector<PDF::SpacedLine> lines =
      pdf.SpaceLines("simple one", width, times);
    CHECK(lines.size() == 2) << lines.size();
    const SpacedLine &line1 = lines[0];
    CHECK(line1.size() == 1);
    CHECK(line1[0].first == "simple");
    const SpacedLine &line2 = lines[1];
    CHECK(line2.size() == 1);
    CHECK(line2[0].first == "one");
  }

  printf("SpaceLine " AGREEN("OK") ".\n");
}

static void MakeSimplePDF() {
  ArcFour rc("pdf");

  static constexpr bool COMPRESS_TEST_PDF = true;

  printf("Create PDF object.\n");
  PDF pdf(PDF::PDF_LETTER_WIDTH, PDF::PDF_LETTER_HEIGHT,
          PDF::Options{.use_compression = COMPRESS_TEST_PDF});

  PDF::Info info;
  sprintf(info.creator, "pdf_test.cc");
  sprintf(info.producer, "Tom 7");
  sprintf(info.title, "It is a test");
  sprintf(info.author, "None");
  sprintf(info.author, "No subject");
  sprintf(info.date, "30 Dec 2023");
  pdf.SetInfo(info);

  std::string pasement_name = pdf.AddTTF("fonts/DFXPasement9px.ttf");

  {
    printf(AWHITE("Shapes page") ".\n");
    PDF::Page *page = pdf.AppendNewPage();

    pdf.AddLine(3, 8, 100, 50, 2.0f,
                PDF_RGB(0x90, 0x40, 0x40));

    pdf.AddQuadraticBezier(3, 8, 100, 50,
                           75, 100,
                           2.0f,
                           PDF_RGB(0x40, 0x90, 0x40));

    pdf.AddCubicBezier(3, 8, 100, 50,
                       75, 5, 15, 120,
                       2.0f,
                       PDF_RGB(0x40, 0x40, 0x90),
                       page);

    pdf.AddCircle(400, 500, 75, 1,
                  PDF_RGB(0x90, 0x60, 0x60),
                  PDF_RGB(0xf0, 0xa0, 0xa0));

    pdf.AddRectangle(PDF::PDF_LETTER_WIDTH - 200,
                     PDF::PDF_LETTER_HEIGHT - 200,
                     100, 125,
                     2.0f,
                     PDF_RGB(0x40, 0x40, 0x10));

    pdf.AddFilledRectangle(PDF::PDF_LETTER_WIDTH - 200,
                           PDF::PDF_LETTER_HEIGHT - 400,
                           100, 125,
                           2.0f,
                           PDF_RGB(0xBB, 0xBB, 0xBB),
                           PDF_RGB(0x40, 0x40, 0x10));

    // TODO: AddCustomPath
    CHECK(pdf.AddPolygon(Star(500, 150, 50, 100, 9), 2.0f,
                         PDF_RGB(0, 0, 0)));

    CHECK(pdf.AddFilledPolygon(Star(500, 150, 25, 75, 9), 1.0f,
                               PDF_RGB(0x70, 0x70, 0)));

    pdf.SetFont(PDF::TIMES_ROMAN);
    CHECK(pdf.AddText("Title of PDF", 72,
                      30, PDF::PDF_LETTER_HEIGHT - 72 - 36,
                      PDF_RGB(0, 0, 0)));
    pdf.SetFont(PDF::HELVETICA);
    CHECK(pdf.AddTextWrap(
              "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
              "sed do eiusmod tempor incididunt ut labore et dolore magna "
              "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
              "ullamco laboris nisi ut aliquip ex ea commodo consequat. "
              "Duis aute irure dolor in reprehenderit in voluptate velit "
              "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
              "occaecat cupidatat non proident, sunt in culpa qui officia "
              "deserunt mollit anim id est laborum.",
              20,
              36, PDF::PDF_LETTER_HEIGHT - 72 - 36 - 48,
              0.0f,
              PDF_RGB(0, 0, 0),
              PDF_INCH_TO_POINT(3.4f),
              PDF::PDF_ALIGN_JUSTIFY));

    pdf.SetFont(PDF::TIMES_ROMAN);
    CHECK(pdf.AddTextRotate(
              "Camera-Ready Copy",
              12,
              350, 36,
              // almost 180 degrees
              3.0,
              PDF_RGB(0x70, 0, 0)));

    pdf.SetFont(pasement_name);

    pdf.AddText("Embedded font text", 16,
                36, 150, PDF_RGB(0, 0, 70));

    pdf.SetFont(PDF::HELVETICA_OBLIQUE);
    pdf.AddSpacedLine({{"Over", -16.0f}, {"lapping", 100.0f}},
                      16,
                      36, 200, PDF_RGB(80, 0, 0));
  }

  {
    printf(AWHITE("Kerning page") ".\n");
    [[maybe_unused]]
    PDF::Page *page = pdf.AppendNewPage();

    float ypos = PDF::PDF_LETTER_HEIGHT - 72 - 36;

    for (const char *filename : FONTS) {
      if (Util::ExistsFile(filename)) {
        float ycolumn2 = ypos;

        const std::string embedded_name = pdf.AddTTF(filename);
        const PDF::FontObj *embedded = pdf.GetFontByName(embedded_name);
        CHECK(embedded != nullptr) << filename << " exists but can't be "
          "loaded?";
        pdf.SetFont(embedded_name);

        CHECK(pdf.AddText(filename, 36,
                          30, ypos,
                          PDF_RGB(0, 0, 0)));
        ypos -= 42.0;

        auto CompareKern = [&](const std::string &s, float size) {
            CHECK(pdf.AddText(s, size,
                              30, ypos,
                              PDF_RGB(0, 0, 0)));

            ypos -= size * 1.1;

            PDF::SpacedLine kerned = embedded->KernText(s);
            CHECK(pdf.AddSpacedLine(kerned, size,
                                    30, ypos,
                                    PDF_RGB(0, 0, 0),
                                    0.0f));
            ypos -= size * 1.2;
          };

        CompareKern("To Await Is To Worry.", 24);
        CompareKern("BoVeX.", 24);
        CompareKern("Mr. Jock, T.V. Quiz Ph.D., bags few lynx.", 12);

        ypos -= 96.0f;

        // Now some narrow text.
        // Built-in-wrapping.
        const char *text =
          "The story: These large sheets of polarizing "
          "material were made specifically for the optical industry. The "
          "manufacturer who made them maintains very high quality "
          "standards. Whenever defects, even minute ones, appear in any "
          "portion of a sheet, the entire piece is rejected. Thus, these "
          "are rejects, because of very minor imperfections, which we can "
          "offer at a fraction of their normal price. The polarizing "
          "material is sandwiched between two clear sheets of butyrate "
          "plastic. It is rigid, easily cut with scissors, and measures "
          ".030\" thick.";
        constexpr double width = 72.0 * 1.5;
        constexpr double size = 9.0f;

        const double left = 4.9 * 72;
        const double padding = 0.10 * 72;
        const double gutter = 0.25 * 72;
        const double right = left + width + gutter;

        auto Rules = [&](float x) {
            pdf.AddLine(x - padding, ycolumn2 + 14,
                        x - padding, 150,
                        1.0f, PDF_RGB(180, 180, 180));

            pdf.AddLine(x + width + padding, ycolumn2 + 14,
                        x + width + padding, 150,
                        1.0f, PDF_RGB(180, 180, 180));

          };

        Rules(left);
        Rules(right);

        pdf.AddTextWrap(text, size,
                        left, ycolumn2,
                        0.0f,
                        PDF_RGB(0, 0, 0),
                        width, PDF::PDF_ALIGN_LEFT);

        std::vector<SpacedLine> spaced =
          pdf.SpaceLines(text, width / size, embedded);
        // TODO: Function that applies standard leading!
        float yy = ycolumn2;
        const float xx = right;
        for (const auto &line : spaced) {
          CHECK(pdf.AddSpacedLine(line, size, xx, yy,
                                  PDF_RGB(0, 0, 0),
                                  0.0f, page)) << filename;
          yy -= size;
        }

       } else {
        printf("Missing " ARED("%s") "\n", filename);
        CHECK(pdf.AddText("Missing " + std::string(filename), 36,
                          30, PDF::PDF_LETTER_HEIGHT - 72 - 36,
                          PDF_RGB(0, 0, 0)));
      }
    }
  }

  {
    printf(AWHITE("Barcode page") ".\n");
    PDF::Page *page = pdf.AppendNewPage();

    static constexpr float GAP = 38.0f;
    static constexpr float BARCODE_HEIGHT = PDF_INCH_TO_POINT(1.0f);
    static constexpr float BARCODE_WIDTH = BARCODE_HEIGHT * std::numbers::phi;
    float y = PDF::PDF_LETTER_HEIGHT - GAP - BARCODE_HEIGHT;
    CHECK(pdf.AddBarcode128a(45, y, BARCODE_WIDTH, BARCODE_HEIGHT,
                             "SIGBOVIK.ORG", PDF_RGB(0, 0, 0)));
    y -= BARCODE_HEIGHT + GAP;

    CHECK(pdf.AddBarcode39(45, y, BARCODE_WIDTH, BARCODE_HEIGHT,
                           "SIGBOVIK.ORG", PDF_RGB(0, 0, 0), page));

    y -= BARCODE_HEIGHT + GAP;

    CHECK(pdf.AddBarcodeEAN13(45, y, BARCODE_WIDTH, BARCODE_HEIGHT,
                              "9780000058898", PDF_RGB(0, 0, 0))) <<
      "Error: " << pdf.GetErr();

    y -= BARCODE_HEIGHT + GAP;

    CHECK(pdf.AddBarcodeUPCA(45, y, BARCODE_WIDTH, BARCODE_HEIGHT,
                             "101234567897", PDF_RGB(0, 0, 0))) <<
      "Error: " << pdf.GetErr();

    y -= BARCODE_HEIGHT + GAP;

    CHECK(pdf.AddBarcodeEAN8(45, y, BARCODE_WIDTH, BARCODE_HEIGHT,
                             "80084321", PDF_RGB(0, 0, 0))) <<
      "Error: " << pdf.GetErr();

    y -= BARCODE_HEIGHT + GAP;

    CHECK(pdf.AddBarcodeUPCE(45, y, BARCODE_WIDTH, BARCODE_HEIGHT,
                             "042100005264", PDF_RGB(0, 0, 0))) <<
      "Error: " << pdf.GetErr();


    // Second column.
    static constexpr float QR_SIZE = BARCODE_WIDTH * 2.0;
    y = PDF::PDF_LETTER_HEIGHT - GAP - QR_SIZE;

    pdf.AddFilledRectangle(256, y, QR_SIZE, QR_SIZE, 0,
                           PDF_RGB(245, 245, 255),
                           PDF_TRANSPARENT);
    CHECK(pdf.AddQRCode(256, y, QR_SIZE, "HTTP:\\\\SIGBOVIK.ORG",
                        PDF_RGB(40, 0, 0)));
    y -= QR_SIZE + GAP;

    static constexpr int qw = 29;
    const float qp = QR_SIZE / (double)qw;
    const float rx = 256 + 2 * qp;
    const float ry = y + 2 * qp;

    CHECK(pdf.AddQRCode(256, y, QR_SIZE,
                        "pls stop invading my personal space"));

    pdf.AddFilledRectangle(rx, ry, qp * 3, qp * 3, 0,
                           PDF_RGB(255, 255, 255),
                           PDF_TRANSPARENT);
    CHECK(pdf.AddQRCode(rx, ry, qp * 3, "so nosy :("));

    y -= QR_SIZE + GAP;



  }

  {
    printf(AWHITE("Image page") ".\n");
    // Barcode page.
    PDF::Page *page = pdf.AppendNewPage();

    ImageRGBA img(160, 80);
    img.Clear32(0xFFFFFFFF);
    img.BlendFilledCircleAA32(50, 40, 31.1, 0x903090AA);

    CHECK(pdf.AddImageRGB(300, 300, 72.0, -1.0,
                          img.IgnoreAlpha())) << "Error: " <<
      pdf.GetErr();

    ImageRGB rand = RandomRGB(&rc, 384, 256);
    CHECK(pdf.AddImageRGB(
              100, PDF::PDF_LETTER_HEIGHT - 72 * 3, -1.0, 72 * 2,
              rand, PDF::CompressionType::PNG, page)) << "Error: " <<
      pdf.GetErr();

    ImageRGB rand2 = RandomRGB(&rc, 384, 256);
    CHECK(pdf.AddImageRGB(
              320, PDF::PDF_LETTER_HEIGHT - 72 * 3, -1.0, 72 * 2,
              rand2, PDF::CompressionType::JPG_10, page)) << "Error: " <<
      pdf.GetErr();

    pdf.SetFont(pasement_name);

    static constexpr char insist_utf8[4] = "\u2014";
    static_assert((uint8_t)insist_utf8[0] == 0xE2);
    static_assert((uint8_t)insist_utf8[1] == 0x80);
    static_assert((uint8_t)insist_utf8[2] == 0x94);
    static_assert((uint8_t)insist_utf8[3] == 0x00);

    CHECK(pdf.AddTextWrap(
              "WELCOME TO THE WWW INTERNET. One of the finest interns of all "
              "time: E.T.. It is illicitly lilliputian. "
              "'lillili.' "
              "@WASTE@ #NOT#. &WANT& !NOT!. ,FONT, \u2014NAUGHT\u2014. "
              "!@#$%^&*()-=",
              18,
              36, 72 * 3,
              0.0f,
              PDF_RGB(0, 0, 0),
              PDF_INCH_TO_POINT(7.0f),
              PDF::PDF_ALIGN_JUSTIFY)) << "Error: " << pdf.GetErr();
  }

  printf("Save it...\n");

  pdf.Save("test.pdf");
  printf("Wrote " AGREEN("test.pdf") "\n");
}

static void MakeMinimalPDF() {
  printf("Create PDF object.\n");
  PDF pdf(PDF::PDF_LETTER_WIDTH, PDF::PDF_LETTER_HEIGHT, PDF::Options{.use_compression = false});

  [[maybe_unused]]
  PDF::Page *page = pdf.AppendNewPage();
  PDF::Info info;
  sprintf(info.creator, "pdf_test.cc");
  sprintf(info.producer, "Tom 7");
  sprintf(info.title, "Minimal PDF");
  sprintf(info.author, "None");
  sprintf(info.author, "No subject");
  sprintf(info.date, "8 Jun 2024");
  pdf.SetInfo(info);

  std::string pasement_name = pdf.AddTTF("fonts/DFXPasement9px.ttf");

  pdf.SetFont(pasement_name);
  CHECK(pdf.AddText("Title of PDF", 72,
                    30, PDF::PDF_LETTER_HEIGHT - 72 - 36,
                    PDF_RGB(0, 0, 0)));

  pdf.Save("minimal.pdf");
  printf("Wrote " AGREEN("minimal.pdf") "\n");

}

int main(int argc, char **argv) {
  ANSI::Init();

  SpaceLine();

  MakeMinimalPDF();
  MakeSimplePDF();

  printf("OK\n");
  return 0;
}
