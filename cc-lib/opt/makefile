
default : opt_test.exe optimizer_test.exe opt-seq_test.exe

BASE=../base/logging.o ../base/stringprintf.o ../arcfour.o

UNAME := $(shell uname -o)
ifeq ($(UNAME),Cygwin)
  # for 64 bits on windows (mingw).
  CXX=x86_64-w64-mingw32-g++
  CC=x86_64-w64-mingw32-g++
  CXXFLAGS=-march=native -pipe -Wall -I.. --std=c++20 -O3 -static $(FLTO)
else
ifeq ($(UNAME),Msys)
  # for 64 bits on windows (msys2).
  CXX=clang++
  CC=clang
  CXXFLAGS=-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -march=native -g -O -Wall -I.. --std=c++20
else
  # Linux
  CXX=g++
  CC=gcc
  # no-psabi suppresses warnings all over STL in GCC 7.1 for ARM (e.g. pi)
  # it can probably be removed in a few years?
  CXXFLAGS=-fsanitize=address -fsanitize=undefined -march=native -g -Wall -I.. --std=c++20 -O3 -Wno-psabi
  SOCK_LFLAGS=
endif
endif

%.o : %.cc *.h makefile
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo -n "."

opt.o : opt.cc opt.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

opt_test.o : opt_test.cc opt.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

opt_test.exe : opt.o opt_test.o $(BASE)
	$(CXX) $(CXXFLAGS) $^ -o $@

optimizer_test.o : optimizer_test.cc opt.h optimizer.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

optimizer_test.exe : opt.o optimizer_test.o $(BASE)
	$(CXX) $(CXXFLAGS) $^ -o $@

large-optimizer_test.exe : opt.o large-optimizer_test.o $(BASE)
	$(CXX) $(CXXFLAGS) $^ -o $@

opt-seq_test.exe : opt-seq_test.o opt-seq.o opt.o $(BASE)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean :
	rm -f *.o *.exe $(BASE)
