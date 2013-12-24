#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Uhd Shifter
# Generated: Mon Dec 16 22:42:15 2013
##################################################
import sys
import datetime

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser

class uhd_shifter(gr.top_block):

    def __init__(self, input_file, output_file):
        gr.top_block.__init__(self, "Uhd Shifter")


        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 256000
        self.offset = offset = -25e3

        ##################################################
        # Blocks
        ##################################################
        self.signal_source = analog.sig_source_c(samp_rate, analog.GR_COS_WAVE, offset, 1, 0)
        self.resampler = filter.rational_resampler_ccc(
                interpolation=96,
                decimation=256,
                taps=None,
                fractional_bw=None,
        )
        self.multiply_const = blocks.multiply_const_vcc((32768, ))
        self.multi = blocks.multiply_vcc(1)
        self.file_source = blocks.file_source(gr.sizeof_gr_complex*1, input_file, False)
        self.file_sink = blocks.file_sink(gr.sizeof_short*1, output_file, False)
        self.file_sink.set_unbuffered(True)
        self.c2is = blocks.complex_to_interleaved_short()

        ##################################################
        # Connections
        ##################################################
        self.connect((self.multi, 0), (self.resampler, 0))
        self.connect((self.file_source, 0), (self.multi, 0))
        self.connect((self.signal_source, 0), (self.multi, 1))
        self.connect((self.c2is, 0), (self.file_sink, 0))
        self.connect((self.multiply_const, 0), (self.c2is, 0))
        self.connect((self.resampler, 0), (self.multiply_const, 0))


# QT sink close method reimplementation

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.signal_source.set_sampling_freq(self.samp_rate)

    def get_offset(self):
        return self.offset

    def set_offset(self, offset):
        self.offset = offset
        self.signal_source.set_frequency(self.offset)

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    parser.add_option("", "--infile", type="string", default="",
                      help="Input file name")
    parser.add_option("", "--outfile", type="string", default="",
                      help="Output file name")
    parser.add_option("-o", "--offset", type="eng_float", default=-25e3,
                      help="Set the offset used for shitfting [default=%default]")
    parser.add_option("-i", "--interp", type="int", default=96,
                      help="Set the interpolation factor [default=%default]")
    parser.add_option("-d", "--decim", type="int", default=256,
                      help="Set the decimation factor [default=%default]")

    (options, args) = parser.parse_args()
    
    if not options.infile:
		print "Must specify input file (see --help)"
		sys.exit()

    if not options.outfile:
		print "Must specify output file (see --help)"
		sys.exit()
    
    print "Parameters:"
    print "    input file:", options.infile
    print "   output file:", options.outfile
    print "        offset:", options.offset
    print " interpolation:", options.interp
    print "    decimation:", options.decim
    
    tb = uhd_shifter(options.infile, options.outfile)
    
    print "Conversion start:", datetime.datetime.now().isoformat()
    tb.start()
    tb.wait()
    print "Conversion end:", datetime.datetime.now().isoformat()
