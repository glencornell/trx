#ifndef DEVICE_H
#define DEVICE_H

void aerror(const char *msg, int r);
int set_alsa_hw(snd_pcm_t *pcm,
		unsigned int rate, unsigned int channels,
		unsigned int buffer);
int set_alsa_sw(snd_pcm_t *pcm);

#endif
