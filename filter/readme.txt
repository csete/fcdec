This is a simple band pass filter application working on S16LE encoded complex
samples. It can be used in between a Funcube Dongle and the console version of
the Funcube telemetry decoder to increase the decoder efficiency.

The filter is also equeipped with an AGC to improve robustness when the signal
is fading.

This is just a quick hack temporary solution until we improve the decoder. Try
"filter -h" for options.

