MAKEFLAGS	+=	--no-builtin-rules
.PHONY		: all clean
.SUFFIXES	:

all		: kvin

kvin	: rekvin.h rekvin.cpp
	c++ -std=c++11 -Wall -Werror -g -I. rekvin.cpp -o kvin

clean	:
	rm -f kvin
