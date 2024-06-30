CPP = g++ -Wall -Werror -Wextra -g -std=c++20

all : pretty encode evel 3v

pretty : pretty.cpp icfp.hpp
	$(CPP) -o '$@' '$<'

encode : encode.cpp icfp.hpp
	$(CPP) -o '$@' '$<'

evel : evel.cpp icfp.hpp
	$(CPP) -o '$@' '$<'

3v : 3v.cpp
	$(CPP) -I../cc-lib -o '$@' '$<' -lgmp
