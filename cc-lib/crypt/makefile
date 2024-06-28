
default : aes_test.exe sha256_test.exe cryptrand_test.exe md5_test.exe hash_bench.exe

CC_LIB_OBJECTS=../base/logging.o ../base/stringprintf.o ../arcfour.o ../ansi.o ../util.o

UNAME := $(shell uname -o)
ifeq ($(UNAME),Cygwin)
  # for 64 bits on windows (mingw).
  CXX=x86_64-w64-mingw32-g++
  CC=x86_64-w64-mingw32-g++
  FLTO=-flto=auto
else
  # Linux and msys2
  CXX=g++
  CC=gcc
endif

CXXFLAGS=-march=native -pipe -Wall -I. -I.. --std=c++20 -O3 -static $(FLTO)

# For linux, others...
# CXX=g++
# CC=gcc
# CXXFLAGS=-I. -I.. --std=c++17 -O2 -DNDEBUG=1

%.o : %.cc *.h makefile
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo -n "."

aes.o : aes.cc aes.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

aes_test.o : aes_test.cc aes.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

aes_test.exe : aes.o aes_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

sha256.o : sha256.cc sha256.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

sha256_test.o : sha256_test.cc sha256.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

sha256_test.exe : sha256.o sha256_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

cryptrand.o : cryptrand.cc cryptrand.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

cryptrand_test.o : cryptrand_test.cc cryptrand.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

cryptrand_test.exe : cryptrand.o cryptrand_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

md5_test.exe : md5_test.o md5.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

hash_bench.exe : sha256.o md5.o hash_bench.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

lfsr_test.exe : lfsr_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean :
	rm -f *.o *.exe
