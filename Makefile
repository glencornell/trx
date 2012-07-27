-include .config

CFLAGS += -MMD -Wall

LDLIBS_ASOUND ?= -lasound
LDLIBS_OPUS ?= -lopus
LDLIBS_ORTP ?= -lortp

LDLIBS += $(LDLIBS_ASOUND) $(LDLIBS_OPUS) $(LDLIBS_ORTP)

.PHONY:		all clean

all:		rx tx

rx:		rx.o device.o sched.o

tx:		tx.o device.o sched.o

clean:
		rm -f *.o *.d tx rx

-include *.d
