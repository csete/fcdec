// FUNcube telemetry decoder test program
// Copyright (c) Phil Ashby, Duncan Hills, Howard Long, 2013
// Released under the terms of the Creative Commons BY-SA-NC
// https://creativecommons.org/licenses/by-nc-sa/3.0/

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#ifdef WIN32
typedef __int8 S8;
typedef __int16 S16;
typedef __int32 S32;
typedef unsigned __int8 U8;
typedef unsigned __int16 U16;
typedef unsigned __int32 U32;
#else
#include <stdint.h>
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef U8 UCHAR;
typedef U8 BYTE;
typedef U16 USHORT;
typedef U32 ULONG;
typedef int BOOL;
typedef float FLOAT;
typedef double DOUBLE;
#define FALSE ((BOOL)0)
#define TRUE ((BOOL)1)
#endif

typedef struct
{
	float fRe;
	float fIm;
} COMPLEXSTRUCT;

#define CIRCULAR_BUFFER_SIZE 16
#define FORWARD_FFT_SIZE    8192
#define INVERSE_FFT_SIZE    8192
#define FORWARD_FFT_RESULT_SIZE 4096
#define INVERSE_FFT_RESULT_SIZE 8192
#define PA_SAMPLE_RATE      (96000)
#define PA_SAMPLE_TYPE      paFloat32
#define FRAMES_PER_BUFFER   (8192)
#define SAMPLE_RATE 9600
#define PI 3.141592653589793238F
#define HALF_PI 1.570796326794896619F
#define TWO_PI 6.283185307179586476F

#define BLOCK_SIZE 256
#define FEC_BLOCK_SIZE 650
#define BIT_RATE 1200
#define BIT_TIME (1.0F/BIT_RATE)
#define SAMPLES_PER_BIT (SAMPLE_RATE/BIT_RATE)
#define RX_MATCHED_FILTER_SIZE 65
#define PREAMBLE_SIZE 768 // Number of bit times for preamble - 5 seconds = 6000 bits, 5200 FEC bits + 32 sync vector bits = 5232, 6000 - 5232 = 768 preamble bits
#define RX_CARRIER_FREQ 1200
#define RX_SINCOS_SIZE 256 // For downconverter sincos table
#define SYNC_VECTOR 0x1acffc1d // Standard CCSDS sync vector
#define SYNC_VECTOR_SIZE 32 // Standard CCSDS sync vector
#define LINE_SIZE 16 // Length of decoded line displayed

// set the number of significant samples for the rolling average to 2 * PA_SAMPLE_RATE for gain control
const float AGC_INV_AVERAGE_FACTOR = 1.0F - (2.0F / (PA_SAMPLE_RATE*2+1));
const float AGC_AVERAGE_FACTOR = 2.0F / (PA_SAMPLE_RATE*2+1);

// set the number of significant samples for the rolling average to 50 for centre frequency
const float CFREQ_INV_AVERAGE_FACTOR = 1.0F - (2.0F / (1+1));
const float CFREQ_AVERAGE_FACTOR = 2.0F / (1+1);

// set the number of significant samples for the rolling average to 10 for psd
const float PSD_INV_AVERAGE_FACTOR = 1.0F - (2.0F / (10+1));
const float PSD_AVERAGE_FACTOR = 2.0F / (10+1);



typedef float SAMPLE;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern int FECDecode(unsigned char *raw,unsigned char *RSdecdata);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef WIN32
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atlstr.h>
#include <atlcoll.h>
#include <atlsafe.h>
#include <atlsync.h>
#include <atlutil.h>
#include "fftw3.h"
using namespace ATL;
#else
#include <fftw3.h>
#endif

const size_t COMPLEX_SAMPLE_BUFFER_BYTES =  sizeof(fftwf_complex) * FORWARD_FFT_SIZE;
const size_t REAL_SAMPLE_BUFFER_BYTES =  sizeof(SAMPLE) * INVERSE_FFT_RESULT_SIZE;
const size_t FFT_RESULT_BUFFER_BYTES = sizeof(fftwf_complex) * FORWARD_FFT_RESULT_SIZE;

