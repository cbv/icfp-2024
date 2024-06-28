#include "atomic-util.h"

// Shared across all EightCounters instances.
thread_local uint8_t internal::EightCounters::idx;
