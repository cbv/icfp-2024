
#ifndef _CC_LIB_SUBPROCESS_H
#define _CC_LIB_SUBPROCESS_H

#include <string>

// NOTE: Only implemented on windows. Always fails on other platforms.
// (TODO: Implement it!)
struct Subprocess {
  // The command line starts with the executable to execute. It
  // can optionally include arguments, which are unstructured.
  // TODO: Utilities for creating command lines from vectors, but
  // note that there is no actually standard way of quoting these
  // (except on Windows, the executable can be double-quoted); the
  // process gets a string to parse, not an array.
  //
  // Returns nullptr if creating the process fails.
  static Subprocess *Create(const std::string &command_line);

  // These functions are not thread-safe and should only be called from
  // one thread. However, it is permissible to have one thread (only) writing
  // while another (only) reads.
  virtual bool Write(const std::string &data) = 0;

  // Reads from both stdout and stderr (but stderr might not work?)
  // When the process terminates, this will return the final output
  // (if non-empty) as a "line" even if it has no newline. Returns
  // false once the process terminates or a read error occurs.
  virtual bool ReadLine(std::string *line) = 0;

  // Terminates the process if it is still running.
  virtual ~Subprocess();
};

#endif
