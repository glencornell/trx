#include <netdb.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <celt/celt.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "defaults.h"
#include "device.h"
#include "sched.h"

#define ENCODED_BYTES 128

typedef int seq_t;

/*
 * Bind to network socket, for sending to the given host and port
 */

static int bind_to_network(const char *host, const char *service,
		struct addrinfo **servinfo, struct addrinfo **theirs)
{
	int r, sd;
	struct addrinfo hints, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	r = getaddrinfo(host, service, &hints, servinfo);
	if (r != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		return -1;
	}

	for (p = *servinfo; p != NULL; p = p->ai_next) {
		sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sd == -1) {
			perror("socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fputs("Failed to bind socket\n", stderr);
		return -1;
	} else {
		*theirs = p;
		return sd;
	}
}

/*
 * Encode a frame of audio and send it over the network
 */

static int send_audio(snd_pcm_t *snd, CELTEncoder *encoder,
		int sd, struct addrinfo *theirs, seq_t *seq)
{
	float *pcm;
	void *packet, *encoded;
	ssize_t z;
	snd_pcm_sframes_t f;

	pcm = alloca(sizeof(float) * DEFAULT_FRAME * DEFAULT_CHANNELS);
	packet = alloca(ENCODED_BYTES + sizeof(uint32_t));
	encoded = packet + sizeof(uint32_t);

	f = snd_pcm_readi(snd, pcm, DEFAULT_FRAME);
	if (f < 0) {
		aerror("snd_pcm_readi", f);
		return -1;
	}
	if (f < DEFAULT_FRAME)
		fprintf(stderr, "Short read, %ld\n", f);

	z = celt_encode_float(encoder, pcm, NULL, encoded, ENCODED_BYTES);
	if (z < 0) {
		fputs("celt_encode_float failed\n", stderr);
		return -1;
	}

	*(uint32_t*)packet = htonl(*seq);
	(*seq)++;

	z = sendto(sd, packet, sizeof(uint32_t) + ENCODED_BYTES, 0,
		theirs->ai_addr, theirs->ai_addrlen);
	if (z == -1) {
		perror("sendto");
		return -1;
	}
	fputc('>', stderr);

	return 0;
}

static int run_tx(snd_pcm_t *snd, CELTEncoder *encoder,
		int sd, struct addrinfo *theirs)
{
	seq_t seq = 0;

	for (;;) {
		int r;

		r = send_audio(snd, encoder, sd, theirs, &seq);
		if (r == -1)
			return -1;
	}
}

static void usage(FILE *fd)
{
	fprintf(fd, "Usage: tx [<parameters>] <host>\n");

	fprintf(fd, "\nAudio device (ALSA) parameters:\n");
	fprintf(fd, "  -d <device>   Device name (default '%s')\n",
		DEFAULT_DEVICE);
	fprintf(fd, "  -m <ms>       Buffer time (milliseconds, default %d)\n",
		DEFAULT_BUFFER);

	fprintf(fd, "\nNetwork parameters:\n");
	fprintf(fd, "  -p <port>     UDP port number or name (default %s)\n",
		DEFAULT_SERVICE);
	fprintf(fd, "  -f <bytes>    Frame size (default %d)\n",
		DEFAULT_FRAME);

	fprintf(fd, "\nEncoding parameters:\n");
	fprintf(fd, "  -r <rate>     Sample rate (default %d)\n",
		DEFAULT_RATE);
	fprintf(fd, "  -c <n>        Number of channels (default %d)\n",
		DEFAULT_CHANNELS);
	fprintf(fd, "  -b <bitrate>  Bitrate (kbps, approx., default %d)\n",
		DEFAULT_BITRATE);
}

int main(int argc, char *argv[])
{
	int sd, r;
	struct addrinfo *servinfo, *theirs;
	snd_pcm_t *snd;
	CELTMode *mode;
	CELTEncoder *encoder;

	/* command-line options */
	const char *device = DEFAULT_DEVICE,
		*service = DEFAULT_SERVICE,
		*host;
	unsigned int buffer = DEFAULT_BUFFER,
		rate = DEFAULT_RATE,
		channels = DEFAULT_CHANNELS;
	size_t frame = DEFAULT_FRAME;
		
	for (;;) {
		int c;

		c = getopt(argc, argv, "c:d:f:m:p:r:");
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			channels = atoi(optarg);
			break;
		case 'd':
			device = optarg;
			break;
		case 'f':
			frame = atol(optarg);
			break;
		case 'm':
			buffer = atoi(optarg);
			break;
		case 'p':
			service = optarg;
			break;
		case 'r':
			rate = atoi(optarg);
			break;
		default:
			usage(stderr);
			return -1;
		}
	}

	if (argv[optind] == NULL) {
		usage(stderr);
		return -1;
	}

	host = argv[optind];

	mode = celt_mode_create(rate, channels, frame, NULL);
	if (mode == NULL) {
		fputs("celt_mode_create failed\n", stderr);
		return -1;
	}
	encoder = celt_encoder_create(mode);
	if (encoder == NULL) {
		fputs("celt_encoder_create failed\n", stderr);
		return -1;
	}

	sd = bind_to_network(host, service, &servinfo, &theirs);
	if (sd == -1)
		return -1;

	r = snd_pcm_open(&snd, device, SND_PCM_STREAM_CAPTURE, 0);
	if (r < 0) {
		aerror("snd_pcm_open", r);
		return -1;
	}
	if (set_alsa_hw(snd, rate, channels, buffer * 1000) == -1)
		return -1;
	if (set_alsa_sw(snd) == -1)
		return -1;

	if (go_realtime() != 0)
		return -1;

	r = run_tx(snd, encoder, sd, theirs);

	freeaddrinfo(servinfo);

	if (snd_pcm_close(snd) < 0)
		abort();

	celt_encoder_destroy(encoder);
	celt_mode_destroy(mode);

	if (close(sd) == -1)
		abort();

	return r;
}
