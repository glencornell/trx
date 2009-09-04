#include <netdb.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <celt/celt.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "defaults.h"
#include "device.h"
#include "jitter.h"
#include "sched.h"

#define ENCODED_BYTES 128

/*
 * Bind to a network socket for receiving packets on the given port
 */

static int listen_on_network(const char *service)
{
	int r, sd;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; /* use my IP */

	r = getaddrinfo(NULL, service, &hints, &servinfo);
	if (r != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		return -1;
	}

	/* Bind to the first one we can */

	for (p = servinfo; p != NULL; p = p->ai_next) {
		sd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK,
				p->ai_protocol);
		if (sd == -1) {
			perror("socket");
			continue;
		}

		if (bind(sd, p->ai_addr, p->ai_addrlen) == -1) {
			if (close(sd) == -1)
				abort();
			perror("bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL) {
		fputs("Failed to bind socket\n", stderr);
		return -1;
	} else {
		return sd;
	}
}

/*
 * Playback audio from the buffer to the audio device
 */

static int play_audio(struct jitbuf_t *jb, CELTDecoder *decoder,
		      snd_pcm_t *snd)
{
	int r;
	void *data;
	float *pcm;
	snd_pcm_sframes_t f;

	pcm = alloca(sizeof(float) * DEFAULT_FRAME * DEFAULT_CHANNELS);

	data = jitbuf_front(jb);
	if (data == NULL) {
		r = celt_decode_float(decoder, NULL, 0, pcm);
	} else {
		r = celt_decode_float(decoder, data, ENCODED_BYTES, pcm);
	}
	if (r != 0) {
		fputs("Error in celt_decode\n", stderr);
		return -1;
	}

	f = snd_pcm_writei(snd, pcm, DEFAULT_FRAME);
	if (f < 0) {
		aerror("snd_pcm_writei", f);
		return -1;
	}
	if (f < DEFAULT_FRAME)
		fprintf(stderr, "Short write %ld\n", f);

	jitbuf_pop(jb);
	free(data);

	return 0;
}

/*
 * Read from the network into the buffer
 */

static int read_from_network(int sd, struct jitbuf_t *jb)
{
	size_t z;
	char buf[32768];
	void *data;
	seq_t seq;

	z = recvfrom(sd, buf, sizeof(buf), 0, NULL, NULL);
	if (z == -1) {
		if (errno == EAGAIN)
			return -EAGAIN;
		perror("recvfrom");
		return -1;
	}
	if (z != sizeof(uint32_t) + ENCODED_BYTES) {
		fputs("small packet", stderr);
		return -1;
	}

	/* First 32-bits is sequence number, rest is data */
	seq = ntohl(*(uint32_t*)buf);
	z -= sizeof(uint32_t);
	assert(z >= 0);
	data = malloc(z);
	memcpy(data, buf + sizeof(uint32_t), z);

	if (jitbuf_push(jb, seq, data) == -1)
		free(data);

	return 0;
}

/*
 * The main loop of receiving audio and playing it back
 */

static int run_rx(int sd, CELTDecoder *decoder, snd_pcm_t *snd)
{
	int r;
	struct jitbuf_t jb;

	jitbuf_init(&jb);

	for (;;) {
		for (;;) {
			r = read_from_network(sd, &jb);
			if (r == -EAGAIN)
				break;
			if (r != 0)
				return -1;
		}

		jitbuf_debug(&jb, stderr);

		if (play_audio(&jb, decoder, snd) == -1)
			return -1;
	}

	jitbuf_clear(&jb);
}

static void usage(FILE *fd)
{
	fprintf(fd, "Usage: rx [<parameters>]\n");

	fprintf(fd, "\nAudio device (ALSA) parameters:\n");
	fprintf(fd, "  -d <device>   Device name (default '%s')\n",
		DEFAULT_DEVICE);
	fprintf(fd, "  -m <ms>       Buffer time (milliseconds, default %d)\n",
		DEFAULT_BUFFER);

	fprintf(fd, "\nNetwork parameters:\n");
	fprintf(fd, "  -p <port>     UDP port number or name (default %s)\n",
		DEFAULT_SERVICE);
}

int main(int argc, char *argv[])
{
	int sd, r;
	snd_pcm_t *snd;
	CELTMode *mode;
	CELTDecoder *decoder;

	/* command-line options */
	const char *device = DEFAULT_DEVICE,
		*service = DEFAULT_SERVICE;
	unsigned int buffer = DEFAULT_BUFFER;

	for (;;) {
		int c;

		c = getopt(argc, argv, "d:m:p:");
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			device = optarg;
			break;
		case 'm':
			buffer = atoi(optarg);
			break;
		case 'p':
			service = optarg;
			break;
		default:
			usage(stderr);
			return -1;
		}
	}

	mode = celt_mode_create(DEFAULT_RATE, DEFAULT_CHANNELS, DEFAULT_FRAME,
				NULL);
	if (mode == NULL) {
		fputs("celt_mode_create failed\n", stderr);
		return -1;
	}
	decoder = celt_decoder_create(mode);
	if (decoder == NULL) {
		fputs("celt_decoder_create failed\n", stderr);
		return -1;
	}

	sd = listen_on_network(service);
	if (sd == -1)
		return -1;

	r = snd_pcm_open(&snd, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (r < 0) {
		aerror("snd_pcm_open", r);
		return -1;
	}
	if (set_alsa_hw(snd, DEFAULT_RATE, DEFAULT_CHANNELS,
			buffer * 1000) == -1)
	{
		return -1;
	}
	if (set_alsa_sw(snd) == -1)
		return -1;

	if (go_realtime() != 0)
		return -1;

	r = run_rx(sd, decoder, snd);

	if (snd_pcm_close(snd) < 0)
		abort();

	celt_decoder_destroy(decoder);
	celt_mode_destroy(mode);

	if (close(sd) == -1)
		abort();

	return r;
}
