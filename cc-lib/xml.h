// Super simple in-memory XML reader.
// MIT-licensed, since this is based on the yxml tokenizer. See xml.cc.

#ifndef _CC_LIB_XML_H
#define _CC_LIB_XML_H

#include <unordered_map>
#include <string>
#include <optional>
#include <vector>

struct XML {
  enum class NodeType {
    Element,
    Text,
  };

  // Attribute order is lost (hash table). Attributes are decoded.
  // Known entities are converted to equivalent UTF-8.
  struct Node {
    NodeType type = NodeType::Text;

    // Elements
    std::string tag;
    std::unordered_map<std::string, std::string> attrs;
    std::vector<Node> children;
    // Text
    std::string contents;
  };

  // Returns the root node of the document, or nullopt if there was a
  // parse error. In the latter case, error is populated with a vague
  // error message if it is non-null.
  static std::optional<Node>
  Parse(const std::string &xml_bytes, std::string *error = nullptr);

};

#endif
