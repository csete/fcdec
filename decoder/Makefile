# FUNcube telemetry decoder program
# Copyright (c) Phil Ashby, Duncan Hills, Howard Long, 2013
# Released under the terms of the Creative Commons BY-SA-NC
# https://creativecommons.org/licenses/by-nc-sa/3.0/

AUDIO_CFLAGS=$(shell pkg-config --cflags fftw3f)
AUDIO_LIBS=$(shell pkg-config --libs fftw3f)

decode: main.o Decoder.o FECDecode.o
	g++ -o $@ main.o Decoder.o FECDecode.o $(AUDIO_LIBS)

clean:
	rm -f *.o decode

%.o: %.c
	gcc -Wall -o $@ -c $< ${AUDIO_CFLAGS}

%.o: %.cpp
	g++ -Wall -o $@ -c $< ${AUDIO_CFLAGS}
