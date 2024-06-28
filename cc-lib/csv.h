#ifndef _CC_LIB_CSV_H
#define _CC_LIB_CSV_H

#include <vector>
#include <string>

struct CSV {

  // Performs unescaping, but otherwise uninterpreted.
  // Returns an empty vector if the file can't be opened or parsed.
  static std::vector<std::vector<std::string>> ParseFile(
      const std::string &filename,
      bool include_header = false);

};

#endif
