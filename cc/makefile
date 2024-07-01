
default : eval.exe

CC_LIB=../cc-lib

UNAME := $(shell uname -o)

ifeq ($(UNAME),Msys)
  # for 64 bits on windows (msys2).
  CXXFLAGS=-O3 -march=native -mtune=native -Wall -Wextra -Wno-unused-parameter -I. -I$(CC_LIB) --std=c++20
  # CXXFLAGS=-fsanitize=address -g -O -march=native -mtune=native -Wall -Wextra -Wno-unused-parameter -I. -I$(CC_LIB) --std=c++20
#  LDFLAGS += -lws2_32
  LDFLAGS += -lgmp
  CXXFLAGS += -DBIG_USE_GMP
else
  # Linux, etc.
  # CXXFLAGS=-fsanitize=address -g -Wall -I. --std=c++20 -O3
  CXXFLAGS=-march=native -Wall -Wno-unused-parameter -I. -I$(CC_LIB) --std=c++20 -O3
  LDFLAGS += -lgmp
  CXXFLAGS += -DBIG_USE_GMP
endif

ifeq ($(shell $(CXX) -v 2>&1 | grep "clang version" | wc -l), 1)
  CLANG=true
  LDFLAGS += -Wl,--stack,167772160
else
  # g++ says it supports --size but it doesn't work
  LDFLAGS += -Wl,-z,stack-size=167772160
  CXXFLAGS += -Wno-missing-field-initializers
endif

CC_LIB_OBJECTS=$(CC_LIB)/util.o $(CC_LIB)/base/logging.o $(CC_LIB)/base/stringprintf.o $(CC_LIB)/ansi.o $(CC_LIB)/bignum/big.o $(CC_LIB)/bignum/bign.o $(CC_LIB)/bignum/bigz.o $(CC_LIB)/bounds.o $(CC_LIB)/arcfour.o
CC_LIB_IMAGE_OBJECTS=$(CC_LIB)/image.o $(CC_LIB)/color-util.o $(CC_LIB)/stb_image.o $(CC_LIB)/stb_image_write.o

%.o : %.cc *.h makefile
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo -n "."


eval.exe : eval.o icfp.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

compress.exe : compress.o icfp.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

spaceship.exe : spaceship.o $(CC_LIB_OBJECTS) $(CC_LIB_IMAGE_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

pp.exe : pp.o icfp.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

ppz3.exe : ppz3.o icfp.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

encode.exe : encode.o icfp.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

icfp_test.exe : icfp_test.o icfp.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

lambdaman.exe : lambdaman.o $(CC_LIB_OBJECTS) $(CC_LIB_IMAGE_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

take-improvements.exe : take-improvements.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean :
	rm -f *.o *.exe $(CC_LIB_OBJECTS) $(CC_LIB_IMAGE_OBJECTS)

