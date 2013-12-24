This is an evolution of the Funcube telemetry decoder posted on the Funcube
Yahoo group on Sep 22, 2013 [1]. It has been modified to work in unix pipelines
allowing both real-time and off-line processing.

The input read from stdin is complex IQ in S16LE format interleaved as IQIQ.

Decoded packets are written to stdout in the same format as submitted to the
Funcube Warehouse.

Various debug info is written to stderr.

The following command line example will take an IQ recording with S16LE samples
run it through the decoder, send decoded packets to packets.txt and debug
output debug.txt:

 $ ./decoder < iq_rec.raw 2>debug.txt 1>packets.txt


Following limitations apply to the present version of the decoder:

 - Sample rate must be 96 ksps S16LE.
 - Signal must be in the upper half of the spectrum.
 - Application may crash if strong DC spike is present.

We are working on eliminating both the sample rate limitation as well as the
crash, until then best performance is achieved if the signal is  15-20 kHz
above the center and run through a bandpass filter.

