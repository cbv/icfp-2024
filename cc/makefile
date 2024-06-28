
default : icfp.exe

UNAME := $(shell uname -o)
ifeq ($(UNAME),Msys)
  # for 64 bits on windows (msys2).
  CXX=clang++
  CC=clang
  # FLTO=-flto=thin
  CXXFLAGS=-fsanitize=address -march=native -Wall -Wextra -Wno-unused-parameter -I. --std=c++20 -g -O $(FLTO)
  SOCK_LFLAGS= -lws2_32
else
  # Linux
  CXX=g++
  CC=gcc
  CXXFLAGS=-fsanitize=address -g -Wall -I. --std=c++20 -O3
  # CXXFLAGS=-march=native -g -Wall -I. --std=c++20 -O3 -Wno-psabi
  SOCK_LFLAGS=
endif

BASE=

%.o : %.cc *.h makefile
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo -n "."


icfp.exe : icfp.o
	$(CXX) $(CXXFLAGS) $^ -o $@


clean :
	rm -f *.o *.exe

