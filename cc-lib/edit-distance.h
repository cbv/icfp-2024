
#ifndef _CC_LIB_EDIT_DISTANCE_H
#define _CC_LIB_EDIT_DISTANCE_H

#include <string>
#include <vector>
#include <functional>

struct EditDistance {

  // For the case where substitutions, insertions, and deletions are
  // all cost 1.
  static int Distance(const std::string &s1, const std::string &s2);

  // Same, but returns min(Distance(s1, s2), threshold). This is much
  // faster for small thresholds.
  static int Ukkonen(const std::string &s1, const std::string &s2,
                     int threshold);

  // Given vectors v1 and v2 with abstract symbols, this finds an
  // alignment with minimal cost and returns it. Not as fast as the
  // above since it needs to do additional bookkeeping.
  // It's also parameterized:
  //   deletion_cost(x)   cost of deleting symbol v1[x]
  //   insertion_cost(x)  cost of inserting symbol v2[x]
  //   subst_cost(x, y)   cost of replacing symbol v1[x] with v2[y]
  // Since this just works with indices into the inputs, we don't
  // even take the input vectors; just their lengths. Typically,
  // substituting a symbol with the same symbol has cost 0.
  //
  // Costs must always be non-negative. Internally we are not
  // careful about overflow, so avoid MAX_INT, etc.
  //
  // The alignment is a series of commands to transform one string
  // into the other. Again, commands only work with indices into the
  // original vectors.
  struct Command {
    // A command describes how to transform v1 into v2. Each step
    // is a deletion (of v1[index1]), an insertion (of v2[index2]),
    // or a substitution (v1[index1] -> v2[index2]). Note that
    // substitutions are also used to represent 'unchanged' symbols.
    bool Delete() const { return index1 != -1 && index2 == -1; }
    bool Insert() const { return index1 == -1 && index2 != -1; }
    bool Subst() const { return index1 != -1 && index2 != -1; }

    int index1 = -1;
    int index2 = -1;
  };
  static std::pair<std::vector<Command>, int> GetAlignment(
      int v1_length, int v2_length,
      // Takes an index into v1.
      const std::function<int(int)> &deletion_cost,
      // Takes an index into v2.
      const std::function<int(int)> &insertion_cost,
      // Takes an index into v1, v2.
      const std::function<int(int, int)> &subst_cost);

  // TODO: Parameterized versions.

 private:
  EditDistance() = delete;
  EditDistance(const EditDistance &) = delete;
};

#endif
