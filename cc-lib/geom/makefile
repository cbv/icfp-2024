
default : latlon_test.exe bezier_test.exe hilbert-curve_test.exe tree-2d_test.exe

CC_LIB_OBJECTS=../base/logging.o ../base/stringprintf.o ../image.o ../stb_image_write.o ../stb_image.o ../arcfour.o

# for 64 bits on windows
# CXX=x86_64-w64-mingw32-g++
# CC=x86_64-w64-mingw32-g++
CXXFLAGS=-march=native -I.. --std=c++20 -O2 -static

%.o : %.cc *.h makefile
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo -n "."

latlon.o : latlon.cc latlon.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

latlon_test.o : latlon_test.cc latlon.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

latlon_test.exe : latlon.o latlon_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

latlon-tree_test.exe : latlon.o latlon-tree_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

bezier.o : bezier.cc bezier.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

bezier_test.o : bezier_test.cc bezier.h
	$(CXX) $(CXXFLAGS) $< -o $@ -c

bezier_test.exe : bezier.o bezier_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

hilbert-curve_test.exe : hilbert-curve_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

tree-2d_test.exe : tree-2d_test.o $(CC_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean :
	rm -f *.o *.exe $(CC_LIB_OBJECTS)
