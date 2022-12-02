bin_PROGRAMS = tx rx

tx_SOURCES = \
	defaults.h \
	device.c \
	device.h \
	notice.h \
	ptt.c \
	ptt.h \
	trx-sched.c \
	trx-sched.h \
	tx.c
tx_CPPFLAGS = $(ALSA_CPPFLAGS) $(OPUS_CPPFLAGS) $(ORTP_CPPFLAGS) $(BCTOOLBOX_CPPFLAGS) $(GPIOD_CPPFLAGS)
tx_LDFLAGS = $(ALSA_LDFLAGS) $(OPUS_LDFLAGS) $(ORTP_LDFLAGS) $(BCTOOLBOX_LDFLAGS) $(GPIOD_LDFLAGS)
tx_LDADD = $(ALSA_LIBS) $(OPUS_LIBS) $(ORTP_LIBS) $(BCTOOLBOX_LIBS) $(GPIOD_LIBS)

rx_SOURCES = \
	defaults.h \
	device.c \
	device.h \
	notice.h \
	trx-sched.c \
	trx-sched.h \
	rx.c
rx_CPPFLAGS = $(ALSA_CPPFLAGS) $(OPUS_CPPFLAGS) $(ORTP_CPPFLAGS) $(BCTOOLBOX_CPPFLAGS)
rx_LDFLAGS = $(ALSA_LDFLAGS) $(OPUS_LDFLAGS) $(ORTP_LDFLAGS) $(BCTOOLBOX_LDFLAGS)
rx_LDADD = $(ALSA_LIBS) $(OPUS_LIBS) $(ORTP_LIBS) $(BCTOOLBOX_LIBS)

