# trx: Realtime audio over IP

## Summary

trx is a simple toolset for broadcasting live audio. It is based on
the Opus codec <http://www.opus-codec.org/> and sends and receives
encoded audio over IP networks.

It can be used for point-to-point audio links or multicast,
eg. private transmitter links or audio distribution. In contrast to
traditional streaming, high quality wideband audio (such as music) can
be sent with low-latency and fast recovery from dropouts.

With quality audio hardware and wired ethernet, a total latency of no
more than a few milliseconds is possible.

## Dependencies

This is only for GNU/Linux systems.  This program depends upon the
following packages:

* ALSA
* oRTP
* Opus

### Installing Dependencies from Debian Systems

```bash
# Install build tools
sudo apt install build-essential autoconf automake git autoconf-archive libtool make 
# Install dependencies
sudo apt install libortp-dev libopus-dev libbctoolbox-dev libasound2-dev libgpiod-dev
```

### Installing Dependencies from Fedora Systems

TODO

## Build

This is a GNU autotools based build system.  To build:

```bash
autoreconf -vi
./configure
make
```

## Run

This package contains an audio transmitter and receiver.  The
transmitter has an optional push-to-talk (PTT) capability.  Both
programs provide run-time help with the `-h` command line flag.  To
gain full perfromance, both programs will request that the program use
the real-time scehduler and lock their processes within physical
memory.  Furthermore, the PTT capability depends upon /dev/input or
GPIO. Therefore, they should run as root.

Example server using multicast on the non-routable local network:

```bash
sudo ./tx -h 224.0.0.17
```

Example receiver using multicast on the non-routable local network:

```bash
sudo ./rx -h 224.0.0.17
```

## TODO

- [ ] Provide latency and jitter metrics
- [ ] Create unit tests
- [ ] Encrypt RTP payloads using SRTP or ZRTP
- [ ] Conjoin `rx` & `tx` into a single application
- [ ] Create Android and iOS apps
- [ ] Explore rnnnoise, echo cancellation and other codecs

## Copyright

(C) Copyright 2020 Mark Hills <mark@xwax.org>

See the COPYING file for licensing terms.

## Attribution

This software was obtained from the following URL:

  http://www.pogo.org.uk/~mark/trx/
