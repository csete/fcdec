This directory contains various tools and utilities used at OZ7SAT and
OZ9AEC for autonomous telemetry reception from the Funcube satellite. They
are made available for reference and inspiration but don't expect that any
of this will work without without proper adjustment to your environment.

A brief description of the tools follows.

--

fcd_capture.txt:

This text file containing various tips and tricks on how to capture audio
from the Funcube Dongle using sox. This file lists the settings that ensure
correct sample rate and format.


submit.sh:

A bash shell script that takes the output of the decoder and submits the
packets to the Funcube Warehouse. You must edit this file and replace MYCALL
with you call sign or user name as used in the warehouse, and enter the
authentication key you have received during registration.

The input is read through stdin or a file specified on the command line.
Please do not change the submission frequency to higher than 1 packet every
5 seconds.


fcd_sequencer.sh:

Bash script used at OZ9AEC to run the recording, filtering, decoding and
submission processes sequentially. The sequence runs in a loop and listens to
commands on a TCP port. The command "start +800" will do the following:

1. Create a new data directory based on the timestamp.
2. Set Funcube Dongle Pro frequency and gain.
3. Record IQ for 800 seconds.
4. Run the IQ data through the filter using different gains (don't ask).
5. Submit the decoded packets to the warehouse.

Please note that you must edit this file before you can use it.

The idea with the TCP command interface is to allow the sequencer to run
in the background. A remote scheduler calculates the passes and triggs the
sequencer using the "start" command. The "exit" command will stop the
sequencer.


fcd_replay.grc

GNU Radio Companion flow graph that can be used to replay a S16LE recoding
and watch the spectrum in real time. Optionally, the spectrum can be
shifted and stored to a new file. Useful for debugging until we get a
better spectrum viewer.


usrp_offline_sequencer.sh

Similar to the fcd_sequencer script but for the USRP. I no longer use nor
maintain this script. It has been included for convenience.


usrp_shift_and_resamnple.py

This GNU Radio python script can be used to shift and resample IQ data
received using a USRP before processing it with the filter and the decoder.
I no longer use nor maintain this script. It has been included for
convenience.

