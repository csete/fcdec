This is a work in progress Funcube telemetry decoder for Linux based on a
Linux port of the library used in the Dashboard application. It was posted
on the Funcube Yahoo group on Sep 22, 2013 [1].

The purpose and scope of this project is to create a set of simple tools
that can be used to create a telemetry receiver station that can run
unattended in the background.

Currently we have this up and running at OZ7SAT using a USRP and at OZ9AEC
using a Funcube Dongle Pro. Both stations use standrard Linux desktop PCs;
however, work is being done on improving the decoder efficiency and
opmtimizing the code to run well on ARM.

The directory has the following layout:

decoder: The decoder from [1], modified to use stdin, stdout and stderr
         instead of .wav files.

fcdctl:  External command line application providing a simple control
         interface to the Funcube Dongle Pro and Pro+.

filter:  A simple bandpass filter working on IQ samples that we use in
         front of the current decoder. The filter also uses stdin, stdout
         and stderr for communicating with the outside world.

tools:   Various utility scripts currently in use at OZ7SAT and OZ9AEC.
         The tools require sox, netcat, awk, md5sum and curl to work.


The procedure for receiving telemetry using a Funcube Dongle Pro is:

  1. Setup frequency and gain of the FCD using fcdctl
  2. Record IQ samples from the FCD (see tools/fcd_capture.txt)
  3. Run the IQ samples trough the bandpass filter
  4. Run the filtered IQ through the decoder
  5. Submit the decoded packets to the Warehouse

The bash script in tools/fcd_sequencer.sh shows an example how these steps
can be run sequentially. Brave souls can also try to run steps 1-4 in a
single pipeline, e.g.

 $ sox <sox-options> | filter | decode | submit.sh

However, this will eliminate any chance of debugging if something goes
wrong. A better option would be to redirect stdout and stderr to separate
files and use "tail -f" on the stdout-file if you want to decode in real-
time.

Alex OZ9AEC

--

[1] http://uk.groups.yahoo.com/neo/groups/FUNcube/conversations/messages/8768

