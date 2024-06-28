CPP = g++ -Wall -Werror -Wextra -g

all : pretty encode eval

pretty : pretty.cpp icfp.hpp
	$(CPP) -o '$@' '$<'

encode : encode.cpp icfp.hpp
	$(CPP) -o '$@' '$<'

eval : eval.cpp icfp.hpp
	$(CPP) -o '$@' '$<'
