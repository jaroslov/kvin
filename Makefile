MAKEFLAGS	+=	--no-builtin-rules
.PHONY		: all clean
.SUFFIXES	:

all		: kvin

kvin	: kvin.h kvin.c
	cc -std=c11 -Wall -Werror -g kvin.c -o kvin

clean	:
	rm -f kvin
