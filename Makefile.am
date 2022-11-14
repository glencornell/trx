bin_PROGRAMS = tx rx

tx_SOURCES = \
	src/defaults.h \
	src/device.c \
	src/device.h \
	src/notice.h \
	src/ptt.c \
	src/ptt.h \
	src/sched.c \
	src/sched.h \
	src/tx.c
tx_CPPFLAGS = $(ALSA_CPPFLAGS) $(OPUS_CPPFLAGS) $(ORTP_CPPFLAGS) $(BCTOOLBOX_CPPFLAGS)
tx_LDFLAGS = $(ALSA_LDFLAGS) $(OPUS_LDFLAGS) $(ORTP_LDFLAGS) $(BCTOOLBOX_LDFLAGS)
tx_LDADD = $(ALSA_LIBS) $(OPUS_LIBS) $(ORTP_LIBS) $(BCTOOLBOX_LIBS)

rx_SOURCES = \
	src/defaults.h \
	src/device.c \
	src/device.h \
	src/notice.h \
	src/sched.c \
	src/sched.h \
	src/rx.c
rx_CPPFLAGS = $(ALSA_CPPFLAGS) $(OPUS_CPPFLAGS) $(ORTP_CPPFLAGS) $(BCTOOLBOX_CPPFLAGS)
rx_LDFLAGS = $(ALSA_LDFLAGS) $(OPUS_LDFLAGS) $(ORTP_LDFLAGS) $(BCTOOLBOX_LDFLAGS)
rx_LDADD = $(ALSA_LIBS) $(OPUS_LIBS) $(ORTP_LIBS) $(BCTOOLBOX_LIBS)

