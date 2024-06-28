
default : eval.exe

CC_LIB=../cc-lib

UNAME := $(shell uname -o)

ifeq ($(UNAME),Msys)
  # for 64 bits on windows (msys2).
  #  CXXFLAGS=-fsanitize=address -march=native -Wall -Wextra -Wno-unused-parameter -I. --std=c++20 -g -O3 $(FLTO)
  CXXFLAGS=-march=native -Wall -Wextra -Wno-unused-parameter -I. -I$(CC_LIB) --std=c++20 -O3
  LDFLAGS += -lws2_32
else
  # Linux, etc.
  # CXXFLAGS=-fsanitize=address -g -Wall -I. --std=c++20 -O3
  CXXFLAGS=-march=native -Wall -Wno-unused-parameter -I. -I$(CC_LIB) --std=c++20 -O3
endif

ifeq ($(shell $(CXX) -v 2>&1 | grep "clang version" | wc -l), 1)
  CLANG=true
  LDFLAGS += -Wl,--stack,167772160
else
  # g++ says it supports --size but it doesn't work
  LDFLAGS += -Wl,-z,stack-size=167772160
  CXXFLAGS += -Wno-missing-field-initializers
endif

CC_LIB_OBJECTS=$(CC_LIB)/util.o $(CC_LIB)/base/logging.o $(CC_LIB)/base/stringprintf.o $(CC_LIB)/ansi.o

%.o : %.cc *.h makefile
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo -n "."


eval.exe : eval.o icfp.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

compress.exe : compress.o icfp.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

clean :
	rm -f *.o *.exe

