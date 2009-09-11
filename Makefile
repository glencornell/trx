-include .config

CFLAGS+=-MMD -Wall

LDLIBS_ASOUND?=-lasound
LDLIBS_CELT?=-lcelt
LDLIBS_ORTP?=-lortp

LDLIBS+=$(LDLIBS_ASOUND) $(LDLIBS_CELT) $(LDLIBS_ORTP)

.PHONY:		all clean

all:		rx tx

rx:		rx.o device.o sched.o

tx:		tx.o device.o sched.o

clean:
		rm -f *.o
		rm -f *.d
		rm -f tx
		rm -f rx

-include *.d
