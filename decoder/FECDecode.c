
 /************************************************************************************/
 /*                                                                                  */
 /*     Reference Decoder for proposed AMSAT P3 Satellite Telemetry FEC Format       */
 /*                                                                                  */
 /*                              Version 1.0                                         */
 /*                                                                                  */
 /*                Last modified 2003 Jun 22 [Sun] 1035 utc                          */
 /*                                                                                  */
 /*                      (C)2003 J R Miller G3RUH                                    */
 /*                                                                                  */
 /*  This program is based on material largely developed by Phil Karn KA9Q over the  */
 /*  period 1997-2002.   Much of it can be found in libraries on his www site at     */
 /*  http://www.ka9q.net/ao40/  and includes notices which are reproduced herein.    */
 /*                                                                                  */
 /************************************************************************************/


/* Defines for environment */
#include <stdio.h>              /* for: fopen  fread  fwrite  fclose  fprintf   */
#include <stdlib.h>             /* for: calloc  free */
#include <string.h>             /* for: memset  memmove  memcpy */

#define Verbose 1               /* Permit some diagnostics; use 0 for none */

/* Defines for RS Decoder(s) */
#ifndef min
#define min(a,b)        ((a) < (b) ? (a) : (b))
#endif
#define NN        255
#define KK        223
#define NROOTS     32           /* NN-KK */
#define FCR       112
#define PRIM       11
#define IPRIM     116
#define A0        (NN)
#define BLOCKSIZE 256           /* Data bytes per frame */
#define RSBLOCKS    2           /* Number of RS decoder blocks */
#define RSPAD      95           /* Unused bytes in block  (KK-BLOCKSIZE/RSBLOCKS) */


/* Defines for Viterbi Decoder for r=1/2 k=7  (to CCSDS convention) */
#define K 7                     /* Constraint length */
#define N 2                     /* Number of symbols per data bit */
#define CPOLYA 0x4f             /* First  convolutional encoder polynomial */
#define CPOLYB 0x6d             /* Second convolutional encoder polynomial */

/* Number of symbols in an FEC block that are */
/* passed to the Viterbi decoder  (320*8 + 6) */
#define NBITS ((BLOCKSIZE+NROOTS*RSBLOCKS)*8+K-1)

/* Defines for Interleaver */
#define ROWS       80           /* Block interleaver rows */
#define COLUMNS    65           /* Block interleaver columns */
#define SYMPBLOCK (ROWS*COLUMNS)        /* Encoded symbols per block */

/* Defines for Re-encoder */
#define SYNC_POLY   0x48        /* Sync vector PN polynomial */

/* Tables moved out to a header file for clarity */
#include "FECTables.h"

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */


/* ----------------------------------------------------------------------------------- */

/* --------------- */
/* Viterbi decoder */
/* --------------- */

/* Viterbi decoder for arbitrary convolutional code
 * viterbi27 and viterbi37 for the r=1/2 and r=1/3 K=7 codes are faster
 * Copyright 1997 Phil Karn, KA9Q
 */

/* This is a bare bones <2,7> Viterbi decoder, adapted from a general purpose model.
 * It is not optimised in any way, neither as to coding (for example the memcopy should
 * be achievable simply by swapping pointers), nor as to simplifying the metric table,
 * nor as to using any machine-specific smarts.  On contemporary machines, in this application,
 * the execution time is negligible.  Many ideas for optimisation are contained in PK's www pages.
 * The real ADC is 8-bit, though in practice 4-bits are actually sufficient.
 * Descriptions of the Viterbi decoder algorithm can be found in virtually any book
 * entitled "Digital Communications".  (JRM)
 */

    int viterbi27(unsigned char *data,  /* Decoded output data */
                  unsigned char *symbols,       /* Raw deinterleaved input symbols */
                  unsigned int nbits)
    {                           /* Number of output bits */
        unsigned int bitcnt = 0;
        int beststate, i, j;
        long cmetric[64], nmetric[64];  /* 2^(K-1) */
        unsigned long *pp;
        long m0, m1, mask;
        int mets[4];            /* 2^N */
        unsigned long *paths;

         pp = paths =
            (unsigned long *) calloc(nbits * 2, sizeof(unsigned long));
        /* Initialize starting metrics to prefer 0 state */
         cmetric[0] = 0;
        for (i = 1; i < 64; i++)
             cmetric[i] = -999999;

        for (;;) {
            /* Read 2 input symbols and compute the 4 branch metrics */
            for (i = 0; i < 4; i++) {
                mets[i] = 0;
                for (j = 0; j < 2; j++) {
                    mets[i] += mettab[(i >> (1 - j)) & 1][symbols[j]];
            }} symbols += 2;
            mask = 1;
            for (i = 0; i < 64; i += 2) {
                int b1, b2;

                b1 = mets[Syms[i]];
                nmetric[i] = m0 = cmetric[i / 2] + b1;
                b2 = mets[Syms[i + 1]];
                b1 -= b2;
                m1 = cmetric[(i / 2) + (1 << (K - 2))] + b2;
                if (m1 > m0) {
                    nmetric[i] = m1;
                    *pp |= mask;
                }
                m0 -= b1;
                nmetric[i + 1] = m0;
                m1 += b1;
                if (m1 > m0) {
                    nmetric[i + 1] = m1;
                    *pp |= mask << 1;
                }
                mask <<= 2;     // assumes sizeof(long)==32bits? hmmn.
                if ((mask & 0xffffffff) == 0) {
                    mask = 1;
                    pp++;
                }
            }
            if (mask != 1)
                pp++;
            if (++bitcnt == nbits) {
                beststate = 0;

                break;
            }
            memcpy(cmetric, nmetric, sizeof(cmetric));
        }
        pp -= 2;
        /* Chain back from terminal state to produce decoded data */
        memset(data, 0, nbits / 8);
        for (i = nbits - K; i >= 0; i--) {
            if (pp[beststate >> 5] & (1L << (beststate & 31))) {
                beststate |= (1 << (K - 1));
                data[i >> 3] |= 0x80 >> (i & 7);
            }
            beststate >>= 1;
            pp -= 2;
        }
        free(paths);
        return 0;
    }

/* ----------------------------------------------------------------------------------- */

/* ---------- */
/* RS Decoder */
/* ---------- */

/* This decoder has evolved extensively through the work of Phil Karn.  It draws
 * on his own ideas and optimisations, and on the work of others.  The lineage
 * is as below, and parts of the authors' notices are included here.  (JRM)

 * Reed-Solomon decoder
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 *
 * Reed-Solomon coding and decoding
 * Phil Karn (karn@ka9q.ampr.org) September 1996
 *
 * This file is derived from the program "new_rs_erasures.c" by Robert
 * Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) and Hari Thirumoorthy
 * (harit@spectra.eng.hawaii.edu), Aug 1995
 * --------------------------------------------------------------------------
 *
 * From the RM-Z & HT program:
 * The encoding and decoding methods are based on the
 * book "Error Control Coding: Fundamentals and Applications",
 * by Lin and Costello, Prentice Hall, 1983, ISBN 0-13-283796-X
 * Portions of this program are from a Reed-Solomon encoder/decoder
 * in C, written by Simon Rockliff (simon@augean.ua.oz.au) on 21/9/89.
 * --------------------------------------------------------------------------
 *
 * From the 1989/1991 SR program (also based on Lin and Costello):
 * This program may be freely modified and/or given to whoever wants it.
 * A condition of such distribution is that the author's contribution be
 * acknowledged by his name being left in the comments heading the program,
 *                               Simon Rockliff, 26th June 1991
 *
 */

    static int mod255(int x) {
        while (x >= 255) {
            x -= 255;
            x = (x >> 8) + (x & 255);
        }
        return x;
    }

    int decode_rs_8(char *data, int *eras_pos, int no_eras) {

        int deg_lambda, el, deg_omega;
        int i, j, r, k;
        unsigned char u, q, tmp, num1, num2, den, discr_r;
        unsigned char lambda[NROOTS + 1], s[NROOTS];    /* Err+Eras Locator poly and syndrome poly */
        unsigned char b[NROOTS + 1], t[NROOTS + 1], omega[NROOTS + 1];
        unsigned char root[NROOTS], reg[NROOTS + 1], loc[NROOTS];
        int syn_error, count;

        /* form the syndromes; i.e., evaluate data(x) at roots of g(x) */
        for (i = 0; i < NROOTS; i++)
            s[i] = data[0];

        for (j = 1; j < NN; j++) {
            for (i = 0; i < NROOTS; i++) {
                if (s[i] == 0) {
                    s[i] = data[j];
                } else {
                    s[i] =
                        data[j] ^
                        ALPHA_TO[mod255
                                 (INDEX_OF[s[i]] + (FCR + i) * PRIM)];
                }
            }
        }

        /* Convert syndromes to index form, checking for nonzero condition */
        syn_error = 0;
        for (i = 0; i < NROOTS; i++) {
            syn_error |= s[i];
            s[i] = INDEX_OF[s[i]];
        }

        if (!syn_error) {
            /* if syndrome is zero, data[] is a codeword and there are no
             * errors to correct. So return data[] unmodified
             */
            count = 0;
            goto finish;
        }
        memset(&lambda[1], 0, NROOTS * sizeof(lambda[0]));
        lambda[0] = 1;

        if (no_eras > 0) {
            /* Init lambda to be the erasure locator polynomial */
            lambda[1] = ALPHA_TO[mod255(PRIM * (NN - 1 - eras_pos[0]))];
            for (i = 1; i < no_eras; i++) {
                u = mod255(PRIM * (NN - 1 - eras_pos[i]));
                for (j = i + 1; j > 0; j--) {
                    tmp = INDEX_OF[lambda[j - 1]];
                    if (tmp != A0)
                        lambda[j] ^= ALPHA_TO[mod255(u + tmp)];
                }
            }
        }
        for (i = 0; i < NROOTS + 1; i++)
            b[i] = INDEX_OF[lambda[i]];

        /*
         * Begin Berlekamp-Massey algorithm to determine error+erasure
         * locator polynomial
         */
        r = no_eras;
        el = no_eras;
        while (++r <= NROOTS) { /* r is the step number */
            /* Compute discrepancy at the r-th step in poly-form */
            discr_r = 0;
            for (i = 0; i < r; i++) {
                if ((lambda[i] != 0) && (s[r - i - 1] != A0)) {
                    discr_r ^=
                        ALPHA_TO[mod255
                                 (INDEX_OF[lambda[i]] + s[r - i - 1])];
                }
            }
            discr_r = INDEX_OF[discr_r];        /* Index form */
            if (discr_r == A0) {
                /* 2 lines below: B(x) <-- x*B(x) */
                memmove(&b[1], b, NROOTS * sizeof(b[0]));
                b[0] = A0;
            } else {
                /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
                t[0] = lambda[0];
                for (i = 0; i < NROOTS; i++) {
                    if (b[i] != A0)
                        t[i + 1] =
                            lambda[i +
                                   1] ^ ALPHA_TO[mod255(discr_r + b[i])];
                    else
                        t[i + 1] = lambda[i + 1];
                }
                if (2 * el <= r + no_eras - 1) {
                    el = r + no_eras - el;
                    /*
                     * 2 lines below: B(x) <-- inv(discr_r) *
                     * lambda(x)
                     */
                    for (i = 0; i <= NROOTS; i++)
                        b[i] =
                            (lambda[i] ==
                             0) ? A0 : mod255(INDEX_OF[lambda[i]] -
                                              discr_r + NN);
                } else {
                    /* 2 lines below: B(x) <-- x*B(x) */
                    memmove(&b[1], b, NROOTS * sizeof(b[0]));
                    b[0] = A0;
                }
                memcpy(lambda, t, (NROOTS + 1) * sizeof(t[0]));
            }
        }

        /* Convert lambda to index form and compute deg(lambda(x)) */
        deg_lambda = 0;
        for (i = 0; i < NROOTS + 1; i++) {
            lambda[i] = INDEX_OF[lambda[i]];
            if (lambda[i] != A0)
                deg_lambda = i;
        }
        /* Find roots of the error+erasure locator polynomial by Chien search */
        memcpy(&reg[1], &lambda[1], NROOTS * sizeof(reg[0]));
        count = 0;              /* Number of roots of lambda(x) */
        for (i = 1, k = IPRIM - 1; i <= NN; i++, k = mod255(k + IPRIM)) {
            q = 1;              /* lambda[0] is always 0 */
            for (j = deg_lambda; j > 0; j--) {
                if (reg[j] != A0) {
                    reg[j] = mod255(reg[j] + j);
                    q ^= ALPHA_TO[reg[j]];
                }
            }
            if (q != 0)
                continue;       /* Not a root */
            /* store root (index-form) and error location number */
            root[count] = i;
            loc[count] = k;
            /* If we've already found max possible roots,
             * abort the search to save time
             */
            if (++count == deg_lambda)
                break;
        }
        if (deg_lambda != count) {
            /*
             * deg(lambda) unequal to number of roots => uncorrectable
             * error detected
             */
            count = -1;
            goto finish;
        }
        /*
         * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
         * x**NROOTS). in index form. Also find deg(omega).
         */
        deg_omega = 0;
        for (i = 0; i < NROOTS; i++) {
            tmp = 0;
            j = (deg_lambda < i) ? deg_lambda : i;
            for (; j >= 0; j--) {
                if ((s[i - j] != A0) && (lambda[j] != A0))
                    tmp ^= ALPHA_TO[mod255(s[i - j] + lambda[j])];
            }
            if (tmp != 0)
                deg_omega = i;
            omega[i] = INDEX_OF[tmp];
        }
        omega[NROOTS] = A0;

        /*
         * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
         * inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form
         */
        for (j = count - 1; j >= 0; j--) {
            num1 = 0;
            for (i = deg_omega; i >= 0; i--) {
                if (omega[i] != A0)
                    num1 ^= ALPHA_TO[mod255(omega[i] + i * root[j])];
            }
            num2 = ALPHA_TO[mod255(root[j] * (FCR - 1) + NN)];
            den = 0;

            /* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
            for (i = min(deg_lambda, NROOTS - 1) & ~1; i >= 0; i -= 2) {
                if (lambda[i + 1] != A0)
                    den ^= ALPHA_TO[mod255(lambda[i + 1] + i * root[j])];
            }
            if (den == 0) {
                count = -1;
                goto finish;
            }
            /* Apply error to data */
            if (num1 != 0) {
                data[loc[j]] ^=
                    ALPHA_TO[mod255
                             (INDEX_OF[num1] + INDEX_OF[num2] + NN -
                              INDEX_OF[den])];
            }
        }
      finish:
        if (eras_pos != NULL) {
            for (i = 0; i < count; i++)
                eras_pos[i] = loc[i];
        }
        return count;
    }

/* ----------------------------------------------------------------------------------- */

/* ---------- */
/* Re-encoder */
/* ---------- */

/* Reference encoder for proposed coded AO-40 telemetry format - v1.0  7 Jan 2002
 * Copyright 2002, Phil Karn, KA9Q
 * This software may be used under the terms of the GNU Public License (GPL)
 */

/* Adapted from  the above enc_ref.c  as used by the spacecraft (JRM) */

    static int Nbytes;          /* Byte counter for encode_data() */
    static int Bindex;          /* Byte counter for interleaver */
    static unsigned char Conv_sr;       /* Convolutional encoder shift register state */
    static unsigned char RS_block[RSBLOCKS][NROOTS];    /* RS parity blocks */
    static unsigned char reencode[SYMPBLOCK];   /* Re-encoded symbols */

    static unsigned char RS_poly[] = {
        249, 59, 66, 4, 43, 126, 251, 97, 30, 3, 213, 50, 66, 170, 5, 24
    };

/* Write one binary channel symbol into the interleaver frame and update the pointers */
    static void interleave_symbol(int c) {
        int row, col;
        col = Bindex / COLUMNS;
        row = Bindex % COLUMNS;
        if (c)
            reencode[row * ROWS + col] = 1;
        Bindex++;
    }

/* Convolutionally encode and interleave one byte */
    static void encode_and_interleave(unsigned char c, int cnt) {
        while (cnt-- != 0) {
            Conv_sr = (Conv_sr << 1) | (c >> 7);
            c <<= 1;
            interleave_symbol(Partab[Conv_sr & CPOLYA]);
            interleave_symbol(!Partab[Conv_sr & CPOLYB]);       /* Second encoder symbol is inverted */
        }
    }

/* Scramble a byte, convolutionally encode and interleave into frame */
    static void scramble_and_encode(unsigned char c) {
        c ^= Scrambler[Nbytes]; /* Scramble byte */
        encode_and_interleave(c, 8);    /* RS encode and place into reencode buffer */
    }


/* Three user's entry points now follow.  They are:

   init_encoder()                   Called once before using system.
   encode_byte(unsigned char c)     Called with each user byte (i.e. 256 calls)
   encode_parity()                  Called 64 times to finish off

*/

/* This function initializes the encoder. */
    static void local_init_encoder(void) {
        int i, j, sr;

        Nbytes = 0;
        Conv_sr = 0;
        Bindex = COLUMNS;       /* Sync vector is in first column; data starts here */

        for (j = 0; j < RSBLOCKS; j++)  /* Flush parity array */
            for (i = 0; i < NROOTS; i++)
                RS_block[j][i] = 0;

        /* Clear re-encoded array */
        for (i = 0; i < 5200; i++)
            reencode[i] = 0;

        /* Generate sync vector, interleave into re-encode array, 1st column */
        sr = 0x7f;
        for (i = 0; i < 65; i++) {
            if (sr & 64)
                reencode[ROWS * i] = 1; /* Every 80th symbol is a sync bit */
            sr = (sr << 1) | Partab[sr & SYNC_POLY];
        }
    }


/* This function is called with each user data byte to be encoded into the
 * current frame. It should be called in sequence 256 times per frame, followed
 * by 64 calls to encode_parity().
 */

    static void local_encode_byte(unsigned char c) {
        unsigned char *rp;      /* RS block pointer */
        int i;
        unsigned char feedback;

        /* Update the appropriate Reed-Solomon codeword */
        rp = RS_block[Nbytes & 1];

        /* Compute feedback term */
        feedback = INDEX_OF[c ^ rp[0]];

        /* If feedback is non-zero, multiply by each generator polynomial coefficient and
         * add to corresponding shift register elements
         */
        if (feedback != A0) {
            int j;

            /* This loop exploits the palindromic nature of the generator polynomial
             * to halve the number of discrete multiplications
             */
            for (j = 0; j < 15; j++) {
                unsigned char t;

                t = ALPHA_TO[mod255(feedback + RS_poly[j])];
                rp[j + 1] ^= t;
                rp[31 - j] ^= t;
            }
            rp[16] ^= ALPHA_TO[mod255(feedback + RS_poly[15])];
        }

        /* Shift 32 byte RS register one position down */
        for (i = 0; i < 31; i++)
            rp[i] = rp[i + 1];

        /* Handle highest order coefficient, which is unity */
        if (feedback != A0) {
            rp[31] = ALPHA_TO[feedback];
        } else {
            rp[31] = 0;
        }
        scramble_and_encode(c);
        Nbytes++;
    }

/* This function should be called 64 times after the 256 data bytes
 * have been passed to update_encoder. Each call scrambles, encodes and
 * interleaves one byte of Reed-Solomon parity.
 */

    static void local_encode_parity(void) {
        unsigned char c;

        c = RS_block[Nbytes & 1][(Nbytes - 256) >> 1];
        scramble_and_encode(c);
        if (++Nbytes == 320) {
            /* Tail off the convolutional encoder (flush) */
            encode_and_interleave(0, 6);
        }
    }

/* Encodes the 256 byte source block RSdecdata[] into 5200 byte block of symbols
 * reencode[].  It has the same format as an off-air received block of symbols.
 */

    void encode_FEC40(unsigned char *reencode,  /* Re-encoded symbols */
                      unsigned char *RSdecdata) {       /* User's source data */
        int i;
        local_init_encoder();
        for (i = 0; i < 256; i++) {
            local_encode_byte(RSdecdata[i]);
        }
        for (i = 0; i < 64; i++) {
            local_encode_parity();
        }
    }


/* ----------------------------------------------------------------------------------- */

/****************************************************************************************************/
/*                                                                                                  */
/*   AMSAT AO-40 FEC Reference Decoder (by JRM)               Input                Output           */
/*   ------------------------------------------               -----------------------------------   */
/*   Step 1: De-interleave                                    raw[5200]       ->  symbols[5132]     */
/*   Step 2: Viterbi decoder                                  symbols[5132]   ->  vitdecdata[320]   */
/*   Step 3: RS decoder                                       vitdecdata[320] ->  RSdecdata[256]    */
/*   Step 4: Option: Re-encode o/p and count channel errors   RSdecdata[256]  ->  reencode[5200]    */
/*                                                                                                  */
/****************************************************************************************************/

#ifdef STANDALONE
    int main(void)
#else
//extern void _AppendDateTime(void);
//extern void _AppendText(char *psz);
    int FECDecode(unsigned char *raw, unsigned char *RSdecdata)
#endif
    {
#ifdef STANDALONE
        unsigned char raw[SYMPBLOCK];   /* 5200 raw received;  sync+symbols  */
#endif
        unsigned char symbols[NBITS * 2 + 65 + 3];      /* de-interleaved sync+symbols */
        unsigned char vitdecdata[(NBITS - 6) / 8];      /* array for Viterbi decoder output data */
//        unsigned char RSdecdata[BLOCKSIZE] ;     /* RS decoder data output (to user)*/
        int nRC = 1;            // Assume we're OK return code

/* Step 1: De-interleaver */
        {
            /* Input  array:  raw   */
            /* Output array:  symbols */
            int col, row;
            int coltop, rowstart;
#ifdef STANDALONE
            FILE *fp;           /* File pointer */

            fp = fopen("fec", "rb");    /* Input 5200 symbols (incl sync) to data array */
            fread(raw, 1, sizeof(raw), fp);
            fclose(fp);
#endif

            //_AppendDateTime();

            coltop = 0;
            for (col = 1; col < ROWS; col++) {  /* Skip first column as it's the sync vector */
                rowstart = 0;
                for (row = 0; row < COLUMNS; row++) {
                    symbols[coltop + row] = raw[rowstart + col];        /* coltop=col*65 ; rowstart=row*80 */
                    rowstart += ROWS;
                }
                coltop += COLUMNS;
            }
        }                       /* end of de-interleaving */


/* Step 2: Viterbi decoder */
/* ----------------------- */
        {
            /* Input  array:  symbols  */
            /* Output array:  vitdecdata */
            viterbi27(vitdecdata, symbols, NBITS);
        }

/* Steps 3: RS decoder */
/* ------------------- */

/* There are two RS decoders, processing 128 bytes each.
 *
 * If both RS decoders are SUCCESSFUL
 * On exit:
 *   rs_failures = 0
 *   rserrs[x]   = number of errors corrected;  range 0 to 16   (x= 0 or 1)
 *   Data output is in array RSdecdata[256].
 *
 * If an RS decoder FAILS
 * On exit:
 *   rs_failures = 1 or 2   (i.e. != 0)
 *   rserrs[x] contains -1
 *   Data output should not be used.
 */

        {
            /* Input  array:  vitdecdata */
            /* Output array:  RSdecdata  */

            char rsblocks[RSBLOCKS][NN];        /*  [2][255] */
            int row, col, di, si;
            int rs_failures;    /* Flag set if errors found */
            int rserrs[RSBLOCKS];       /* Count of errors in each RS codeword */

            //FILE *fp ;                            /* Needed if you save o/p to file */

            /* Interleave into Reed Solomon codeblocks */
            memset(rsblocks, 0, sizeof(rsblocks));      /* Zero rsblocks array */
            di = 0;
            si = 0;
            for (col = RSPAD; col < NN; col++) {
                for (row = 0; row < RSBLOCKS; row++) {
                    rsblocks[row][col] = vitdecdata[di++] ^ Scrambler[si++];    /* Remove scrambling */
                }
            }
            /* Run RS-decoder(s) */
            rs_failures = 0;
            for (row = 0; row < RSBLOCKS; row++) {
                rserrs[row] = decode_rs_8(rsblocks[row], NULL, 0);
                rs_failures += (rserrs[row] == -1);
            }

            /* If frame decoded OK, deinterleave data from RS codeword(s), and save file */
            if (!rs_failures) {
                int j = 0;

                for (col = RSPAD; col < KK; col++) {
                    for (row = 0; row < RSBLOCKS; row++) {
                        RSdecdata[j++] = rsblocks[row][col];
                    }
                }
                /* and save out succesfully RS-decoded data */
                //fp=fopen("RSdecdataC","wb");                       /* Output 256 bytes of user's data */
                //fwrite(RSdecdata,1,sizeof(RSdecdata),fp);

                //fclose(fp);
            }

            /* Print RS-decode status summary */
            {
                //char sz[80];

                if (Verbose)
                {
                    fprintf(stderr, "RS byte corrections: ");
                    //sprintf(sz, "RS byte corrections: ");
                    //printf("%s", sz);
                }
                for (row = 0; row < RSBLOCKS; row++)
                {
                    if (rserrs[row] != -1)
                    {
                        if (Verbose)
                        {
                            fprintf(stderr, " %d  ", rserrs[row]);
                            //sprintf(sz, " %d  ", rserrs[row]);
                            //printf("%s", sz);
                        }
                    }
                    else
                    {
                        if (Verbose)
                        {
                            fprintf(stderr, "FAIL ");
                            //sprintf(sz, "FAIL ");
                            //puts(sz);
                        }
                        nRC = 0;
                    }
                }
            }
        }                       /* end of rs section */


/* Step 4: Optional: Re-encode o/p and count channel errors  */
/* --------------------------------------------------------  */

        {
            int errors = 0;
            int i;
            /* Input  array:  RSdecdata */
            /* Output array:  reencode  */

            encode_FEC40(reencode, RSdecdata);  /* Re-encode in AO-40 FEC format */

            /* Count the channel errors */
            errors = 0;
            for (i = 0; i < SYMPBLOCK; i++)
                if (reencode[i] != (raw[i] >> 7)) {
                    errors++;
                }

            if (Verbose) {
                //char sz[80];
                //sprintf(sz,"Channel symbol errors: %d (%.3g%%)",errors,100.*errors/SYMPBLOCK);
                fprintf(stderr, " Channel symbol errors: %d (%.3g%%)\n", errors,
                        100. * errors / SYMPBLOCK);
                //_AppendText(sz);
            }

        }                       /* end of reencode section */

/* --------------------------------------------------------*/

        return nRC;
    }                           /*end of main */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
