
default : icfp.exe

UNAME := $(shell uname -o)

ifeq ($(UNAME),Msys)
  # for 64 bits on windows (msys2).
  # FLTO=-flto=thin
  CXXFLAGS=-fsanitize=address -march=native -Wall -Wextra -Wno-unused-parameter -I. --std=c++20 -g -O3 $(FLTO)
  LDFLAGS += -lws2_32
else
  # Linux, etc.
  CXXFLAGS=-fsanitize=address -g -Wall -I. --std=c++20 -O3
  # CXXFLAGS=-march=native -g -Wall -I. --std=c++20 -O3
endif

ifeq ($(shell $(CXX) -v 2>&1 | grep "clang version" | wc -l), 1)
  CLANG=true
  LDFLAGS += -Wl,--stack,167772160
else
  # g++ says it supports --size but it doesn't work
  LDFLAGS += -Wl,-z,stack-size=167772160
  CXXFLAGS += -Wno-missing-field-initializers
endif

# BASE=

%.o : %.cc *.h makefile
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo -n "."


icfp.exe : icfp.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@


clean :
	rm -f *.o *.exe

