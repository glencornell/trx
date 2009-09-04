-include .config

CFLAGS+=-MMD -Wall

.PHONY:		all clean

all:		rx tx

rx:		rx.o device.o jitter.o sched.o
rx:		LDLIBS+=-lcelt -lasound

tx:		tx.o device.o sched.o
tx:		LDLIBS+=-lcelt -lasound

test-jitter:	test-jitter.o jitter.o

clean:
		rm -f *.o
		rm -f *.d
		rm -f tx
		rm -f rx
		rm -f test-jitter

-include *.d
