#include <netdb.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <celt/celt.h>
#include <ortp/ortp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "defaults.h"
#include "device.h"
#include "notice.h"
#include "sched.h"

static unsigned int verbose = DEFAULT_VERBOSE;

static RtpSession* create_rtp_recv(const char *addr_desc, const int port,
		unsigned int jitter)
{
	RtpSession *session;

	session = rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_scheduling_mode(session, 0);
	rtp_session_set_blocking_mode(session, 0);
	rtp_session_set_local_addr(session, addr_desc, port);
	rtp_session_set_connected_mode(session, FALSE);
	rtp_session_enable_adaptive_jitter_compensation(session, TRUE);
	rtp_session_set_jitter_compensation(session, jitter); /* ms */
	if (rtp_session_set_payload_type(session, 0) != 0)
		abort();
	if (rtp_session_signal_connect(session, "timestamp_jump",
			(RtpCallback)rtp_session_resync, 0) != 0)
	{
		abort();
	}

	return session;
}

static int play_one_frame(void *packet,
		size_t len,
		CELTDecoder *decoder,
		snd_pcm_t *snd,
		const unsigned int channels,
		const snd_pcm_uframes_t samples)
{
	int r;
	float *pcm;
	snd_pcm_sframes_t f;

	pcm = alloca(sizeof(float) * samples * channels);

	if (packet == NULL) {
		r = celt_decode_float(decoder, NULL, 0, pcm);
	} else {
		r = celt_decode_float(decoder, packet, len, pcm);
	}
	if (r != 0) {
		fputs("Error in celt_decode\n", stderr);
		return -1;
	}

	f = snd_pcm_writei(snd, pcm, samples);
	if (f < 0) {
		aerror("snd_pcm_writei", f);
		return -1;
	}
	if (f < samples)
		fprintf(stderr, "Short write %ld\n", f);

	return 0;
}

static int run_rx(RtpSession *session,
		CELTDecoder *decoder,
		snd_pcm_t *snd,
		const unsigned int channels,
		const snd_pcm_uframes_t frame)
{
	int ts = 0;

	for (;;) {
		int r, have_more;
		char buf[32768];
		void *packet;

		r = rtp_session_recv_with_ts(session, (uint8_t*)buf,
				sizeof(buf), ts, &have_more);
		assert(r >= 0);
		assert(have_more == 0);
		if (r == 0) {
			packet = NULL;
			if (verbose > 1)
				fputc('#', stderr);
		} else {
			packet = buf;
			if (verbose > 1)
				fputc('.', stderr);
		}

		r = play_one_frame(packet, r, decoder, snd, channels, frame);
		if (r == -1)
			return -1;

		ts += frame;
	}
}

static void usage(FILE *fd)
{
	fprintf(fd, "Usage: rx [<parameters>]\n");

	fprintf(fd, "\nAudio device (ALSA) parameters:\n");
	fprintf(fd, "  -d <dev>    Device name (default '%s')\n",
		DEFAULT_DEVICE);
	fprintf(fd, "  -m <ms>     Buffer time (milliseconds, default %d)\n",
		DEFAULT_BUFFER);

	fprintf(fd, "\nNetwork parameters:\n");
	fprintf(fd, "  -h <addr>   IP address to listen on (default %s)\n",
		DEFAULT_ADDR);
	fprintf(fd, "  -p <port>   UDP port number (default %d)\n",
		DEFAULT_PORT);
	fprintf(fd, "  -j <ms>     Jitter buffer (milliseconds, default %d)\n",
		DEFAULT_JITTER);

	fprintf(fd, "\nEncoding parameters (must match sender):\n");
	fprintf(fd, "  -r <rate>   Sample rate (default %d)\n",
		DEFAULT_RATE);
	fprintf(fd, "  -c <n>      Number of channels (default %d)\n",
		DEFAULT_CHANNELS);
	fprintf(fd, "  -f <bytes>  Frame size (default %d)\n",
		DEFAULT_FRAME);

	fprintf(fd, "\nDisplay parameters:\n");
	fprintf(fd, "  -v <n>      Verbosity level (default %d)\n",
		DEFAULT_VERBOSE);
}

int main(int argc, char *argv[])
{
	int r;
	snd_pcm_t *snd;
	CELTMode *mode;
	CELTDecoder *decoder;
	RtpSession *session;

	/* command-line options */
	const char *device = DEFAULT_DEVICE,
		*addr = DEFAULT_ADDR;
	unsigned int buffer = DEFAULT_BUFFER,
		rate = DEFAULT_RATE,
		frame = DEFAULT_FRAME,
		jitter = DEFAULT_JITTER,
		channels = DEFAULT_CHANNELS,
		port = DEFAULT_PORT;

	fputs("rx " COPYRIGHT "\n", stderr);

	for (;;) {
		int c;

		c = getopt(argc, argv, "d:f:h:j:m:p:v:");
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			device = optarg;
			break;
		case 'f':
			frame = atoi(optarg);
			break;
		case 'h':
			addr = optarg;
			break;
		case 'j':
			jitter = atoi(optarg);
			break;
		case 'm':
			buffer = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'v':
			verbose = atoi(optarg);
			break;
		default:
			usage(stderr);
			return -1;
		}
	}

	mode = celt_mode_create(rate, channels, frame, NULL);
	if (mode == NULL) {
		fputs("celt_mode_create failed\n", stderr);
		return -1;
	}
	decoder = celt_decoder_create(mode);
	if (decoder == NULL) {
		fputs("celt_decoder_create failed\n", stderr);
		return -1;
	}

	if (go_realtime() != 0)
		return -1;

	ortp_init();
	ortp_scheduler_init();
	session = create_rtp_recv(addr, port, jitter);
	assert(session != NULL);

	r = snd_pcm_open(&snd, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (r < 0) {
		aerror("snd_pcm_open", r);
		return -1;
	}
	if (set_alsa_hw(snd, rate, channels, buffer * 1000) == -1)
		return -1;
	if (set_alsa_sw(snd) == -1)
		return -1;

	r = run_rx(session, decoder, snd, channels, frame);

	if (snd_pcm_close(snd) < 0)
		abort();

	rtp_session_destroy(session);
	ortp_exit();
	ortp_global_stats_display();

	celt_decoder_destroy(decoder);
	celt_mode_destroy(mode);

	return r;
}
