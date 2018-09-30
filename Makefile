MAKEFLAGS	+=	--no-builtin-rules
.PHONY		: all clean
.SUFFIXES	:

all			: ./bin/kvin

./bin/kvin	: rekvin.h rekvin.cpp
	@if [ ! -d ./bin ]; then mkdir -p ./bin; fi;
	c++ -std=c++11 -Wall -Werror -g -I. rekvin.cpp -o ./bin/kvin

clean		:
	rm -rf ./bin/kvin
