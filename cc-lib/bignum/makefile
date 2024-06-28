
default : big_test.exe big-overloads_test.exe big-nogmp_test.exe big-overloads-nogmp_test.exe

UNAME := $(shell uname -o)
ifeq ($(UNAME),Cygwin)
  # for 64 bits on windows (mingw).
  # For locally compiled libgmp; pick the right directory:
  LIBGMP=/c/code/gmp-6.3.0
  GMP_LFLAGS=$(LIBGMP)/.libs/libgmp.a
  GMP_CFLAGS=-I$(LIBGMP)/ -DBIG_USE_GMP

  CXX=x86_64-w64-mingw32-g++
  CC=x86_64-w64-mingw32-g++
  LFLAGS=-flto=auto -static
else
  # Linux
  CXX=g++
  CC=gcc
  GMP_LFLAGS=-lgmp
  GMP_CFLAGS=-DBIG_USE_GMP
  PLATFORM_CXXFLAGS=-fsanitize=address -fsanitize=undefined
  LFLAGS=-flto=auto
endif

CXXFLAGS= $(PLATFORM_CXXFLAGS) -march=native -Wall -I.. --std=c++20 -g -O3

# Just used for testing; bignum itself should not depend on cc-lib.
# this assumes that cc-lib doesn't depend on GMP flags
CCLIB_OBJECTS=../base/logging.o ../base/stringprintf.o ../util.o ../ansi.o

%.o : %.cc *.h makefile
	$(CXX) $(GMP_CFLAGS) $(CXXFLAGS) $< -o $@ -c

# with GMP; don't need bigz.o bign.o bigq.o
big_test.exe : big.o big_test.o $(CCLIB_OBJECTS)
	$(CXX) $(GMP_CFLAGS) $(CXXFLAGS) $^ -o $@ $(GMP_LFLAGS) $(LFLAGS)

# bigz.o bign.o bigq.o
big-overloads_test.exe : big.o big-overloads_test.o $(CCLIB_OBJECTS)
	$(CXX) $(GMP_CFLAGS) $(CXXFLAGS) $^ -o $@ $(GMP_LFLAGS) $(LFLAGS)

# Without GMP

%-nogmp.o : %.cc *.h makefile
	$(CXX) $(CXXFLAGS) $< -o $@ -c

big-nogmp_test.exe : bigz-nogmp.o bign-nogmp.o bigq-nogmp.o big-nogmp.o big_test-nogmp.o $(CCLIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LFLAGS)

big-overloads-nogmp_test.exe : bigz-nogmp.o bign-nogmp.o bigq-nogmp.o big-nogmp.o big-overloads_test-nogmp.o $(CCLIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LFLAGS)

clean :
	rm -f *.o *.exe
