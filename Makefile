CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

tmcc: $(OBJS)
				$(CC) -o tmcc $(OBJS) $(LDFLAGS)

$(OBJS): tmcc.h

test: tmcc
				./test.sh

clean:
				rm -f tmcc *.o *~ tmp*

.PHONY: test clean