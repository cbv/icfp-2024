
default : eval.exe

UNAME := $(shell uname -o)

ifeq ($(UNAME),Msys)
  # for 64 bits on windows (msys2).
  #  CXXFLAGS=-fsanitize=address -march=native -Wall -Wextra -Wno-unused-parameter -I. --std=c++20 -g -O3 $(FLTO)
  CXXFLAGS=-march=native -Wall -Wextra -Wno-unused-parameter -I. --std=c++20 -O3
  LDFLAGS += -lws2_32
else
  # Linux, etc.
  # CXXFLAGS=-fsanitize=address -g -Wall -I. --std=c++20 -O3
  CXXFLAGS=-march=native -Wall -Wno-unused-parameter -I. --std=c++20 -O3
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


eval.exe : eval.o icfp.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@


clean :
	rm -f *.o *.exe

