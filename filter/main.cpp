//
// filter: Simple bandpass filter with AGC for Funcube telemetry receiver.
//         Complex input in S16LE format.
//         Complex output in float (default) or S16LE format
//
// This Software is released under the "Simplified BSD License"
// Copyright 2013 Alexandru Csete. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY Alexandru Csete "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
// NO EVENT SHALL Alexandru Csete OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are
// those of the authors and should not be interpreted as representing official
// policies, either expressed or implied, of Alexandru Csete.
//
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "datatypes.h"
#include "fastfir.h"
#include "agc.h"

static void help(void)
{
    static const char help_string[] =
        "\n Usage: filter [options]\n"
        "\n Possible options are:\n"
        "  -s\tSet sample rate (default is 96 ksps).\n"
        "  -o\tSet filter offset in Hz, or use k, M, G suffix (default is 10k).\n"
        "  -w\tSet filter width in Hz, or use k, M, G suffix (default is 16k).\n"
        "  -i\tUse integer S16LE output format (default is to use float).\n"
        "  -d\tSet AGC decay in msec (default is 100 and 0 disables AGC).\n"
        "  -g\tSet manual gain in dB (used when AGC is off).\n"
        "  -h\tThis help message\n"
        "\nThe default parameters are set to work when the Funcube Dongle is tuned to"
        "\n145.925 MHz. The nominal beacon frequency is 145.935 MHz (145.938 MHz at AOS"
        "\nand 145.932 MHz at LOS).\n"
        "\nRaw input samples are read from stdin. Filtered output is sent to stdout."
        "\nInput is S16LE format while output can be float or S16LE. The interleaving"
        "\norder is first I then Q.\n"
        "\nDebug and error messages are printed on stderr.\n";

    printf("%s", help_string);
}


/** \brief Parse frequency string.
 *  \param freq_str The frequency string.
 *  \returns The frequency in Hz or 0 in case of error..
 */
TYPEREAL parse_freq(char *freq_str)
{
    size_t n = strlen(freq_str);
    double multi = 1.0;

    if (n == 0)
        return 0;

    switch (freq_str[n-1])
    {
        case 'k':
        case 'K':
            multi = 1.e3;
            break;

        case 'M':
            multi = 1.e6;
            break;

        case 'G':
            multi = 1.e9;
            break;
    }

    return (TYPEREAL)(multi * atof(freq_str));
}



int main(int argc, char **argv)
{
    TYPEREAL sample_rate = 96.e3;
    TYPEREAL filter_offset = 10.e3;
    TYPEREAL filter_width  = 16.e3;
    TYPEREAL manual_gain = 0.0;      // Manual gain in dB when AGC is off
    int      agc_decay = 100;        // AGC decay rate in ms  

    bool use_integer_out = false;

    int option;

    if (argc > 1)
    {
        while ((option = getopt(argc, argv, "s:o:w:d:g:hi")) != -1)
        {
            switch (option)
            {
                case 's':
                    sample_rate = parse_freq(optarg);
                    if (sample_rate == 0.0)
                    {
                        fprintf(stderr, "Invalid sample rate: %s\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case 'o':
                    filter_offset = parse_freq(optarg);
                    if (filter_offset == 0.0)
                    {
                        fprintf(stderr, "Invalid filter offset: %s\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case 'w':
                    filter_width = parse_freq(optarg);
                    if (filter_width == 0.0)
                    {
                        fprintf(stderr, "Invalid filter width: %s\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case 'd':
                    agc_decay = atoi(optarg);
                    // valid range 20 to 5000 and 0 to disable AGC
                    if ((agc_decay > 5000) || ((agc_decay < 20) && (agc_decay != 0)))
                    {
                        fprintf(stderr, "Invalid AGC decay: %s\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                    break;

                case 'g':
                    manual_gain = (TYPEREAL)atof(optarg);
                    break;

                case 'i':
                    use_integer_out = true;
                    break;

                case 'h':
                    help();
                    exit(EXIT_SUCCESS);

                default:
                    help();
                    exit(EXIT_FAILURE);
            }

        }
    }

    // TODO: a few consistency checks between sample rate and filter parameters
    fprintf(stderr, "Sample rate: %.0f Hz\n", sample_rate);
    fprintf(stderr, "Filter offset / width: %.0f Hz / %.0f Hz\n",
            filter_offset, filter_width);
    fprintf(stderr, "AGC decay: %d ms\n", agc_decay);
    fprintf(stderr, "Manual gain: %.1f dB\n", manual_gain);

    CFastFIR filter;
    filter.SetupParameters(-filter_width / 2.0, filter_width / 2.0,
                           filter_offset, sample_rate);

    CAgc agc;
    agc.SetParameters((agc_decay!=0), false, -50.0, manual_gain,
            0, agc_decay != 0 ? agc_decay : 100, sample_rate);

    fprintf(stderr, "Starting filter. Press ctrl-c to exit...\n");

#define NUM_SAMP    8192
#define BUFFER_SIZE 2 * NUM_SAMP
    short    input_buffer[BUFFER_SIZE];
    TYPECPX  pre_filter[NUM_SAMP];
    TYPECPX  post_filter[NUM_SAMP];
    TYPECPX  post_agc[NUM_SAMP];
    short    output_buffer_i[BUFFER_SIZE];
    size_t   read, written, i;


    while (true)
    {
        read = fread(input_buffer, sizeof(short), BUFFER_SIZE, stdin);
        if (read <= 0)
        {
            return 0;
        }
        else if (read != BUFFER_SIZE)
        {
            fprintf(stderr, "Read %ld bytes (asked for %d)\n",
                    read, BUFFER_SIZE);
        }

        // convert input buffer to float complex
        for (i = 0; i < read / 2; i++)
        {
            pre_filter[i].re = (TYPEREAL)input_buffer[2*i];
            pre_filter[i].im = (TYPEREAL)input_buffer[2*i+1];
        }

        // process data
        filter.ProcessData(read/2, pre_filter, post_filter);
        agc.ProcessData(read/2, post_filter, post_agc);

        if (use_integer_out)
        {
            // convert output buffer to shorts and write to stdout
            for (i = 0; i < read/2; i++)
            {
                output_buffer_i[2*i]   = (short) post_agc[i].re;
                output_buffer_i[2*i+1] = (short) post_agc[i].im;
            }
            written = fwrite(output_buffer_i, sizeof(short), read, stdout);
        }
        else
        {
            // Scale output to -1.0 ... 1.0
            for (i = 0; i < read/2; i++)
            {
                post_agc[i].re /= 32767.0;
                post_agc[i].im /= 32767.0;
            }
            written = fwrite(post_agc, sizeof(TYPEREAL), read, stdout);
        }

        if (written != read)
            fprintf(stderr, "Wrote %ld samples (tried to write %ld)\n",
                    written, read);
    }

    return 0;
}

