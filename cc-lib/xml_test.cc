
#include "xml.h"

#include "base/stringprintf.h"
#include "base/logging.h"
#include "arcfour.h"
#include "randutil.h"

using namespace std;

using NodeType = XML::NodeType;
using Node = XML::Node;

// TODO: Test entities!

static void TestMinimal() {
  string error;
  optional<Node> node = XML::Parse(R"(
<?xml version="1.0" encoding="UTF-8"?>
<empty></empty>
)", &error);

  CHECK(node.has_value()) << error;
  CHECK(node->type == NodeType::Element);
  CHECK(node->tag == "empty");
  CHECK(node->children.empty());
  CHECK(node->attrs.empty());
  CHECK(node->contents.empty());
}

static void TestDuplicateAttr() {
  string error;
  optional<Node> node = XML::Parse(R"(
<?xml version="1.0" encoding="UTF-8"?>
<test dup="yes" singleton="ok" dup="again">
</test>
)", &error);

  CHECK(!node.has_value());
  CHECK(error.find("Duplicate attribute dup") != string::npos);
}


static void TestSimpleDoc() {
  string error;
  optional<Node> onode = XML::Parse(R"(
<?xml version="1.0" encoding="UTF-8"?>
<test attr="yes">hello.<subtag>ok</subtag><void zoo="hi"/>bye</test>
)", &error);

  CHECK(onode.has_value()) << error;

  Node &node = onode.value();
  CHECK(node.type == NodeType::Element);
  CHECK(node.tag == "test");
  CHECK(node.attrs.size() == 1);
  CHECK(node.attrs["attr"] == "yes");
  CHECK(node.children.size() == 4);
  Node &c0 = node.children[0];
  Node &c1 = node.children[1];
  Node &c2 = node.children[2];
  Node &c3 = node.children[3];
  CHECK(c0.type == NodeType::Text);
  CHECK(c0.contents == "hello.");
  CHECK(c1.type == NodeType::Element);
  CHECK(c1.tag == "subtag");
  CHECK(c1.children.size() == 1);
  CHECK(c1.children[0].contents == "ok");
  CHECK(c2.type == NodeType::Element);
  CHECK(c2.attrs.size() == 1);
  CHECK(c2.attrs["zoo"] == "hi");
  CHECK(c2.children.empty());
  CHECK(c3.type == NodeType::Text);
  CHECK(c3.contents == "bye");
}


int main(int argc, char **argv) {
  TestMinimal();
  TestDuplicateAttr();
  TestSimpleDoc();

  printf("OK\n");
  return 0;
}
