// FUNcube decoder test program
// Copyright (c) Phil Ashby, 2013
// Released under the terms of the Creative Commons BY-SA-NC
// https://creativecommons.org/licenses/by-nc-sa/3.0/

#include <stdio.h>
#include <unistd.h>
#include <sndfile.h>
#include <sys/time.h>
#include <math.h>
#include "stdafx.h"
#include "Decoder.h"

#define PROCESSING_PERIOD_USEC	((int) (((long)FORWARD_FFT_SIZE*1000000L)/(long)PA_SAMPLE_RATE) )

class CTryDecode : public IDecodeResult {
public:
	CTryDecode(int, char **);
	~CTryDecode(void);
	void go(void);
	// IDecodeResult method
	void OnDecodeSuccess(U8* result, U32 length, U32 centreBin);

private:
	float min(float a, float b) { return a<b ? a : b; }
	float max(float a, float b) { return a<b ? b : a; }
	CDecoder *_decoder;
	fftwf_complex *_fftIn, *_fftOut;
	fftwf_plan _fftPlan;
	float _averageCentreBin;
	float _psd[FORWARD_FFT_RESULT_SIZE];
	float _avePsd[FORWARD_FFT_RESULT_SIZE];
	float _averagePeekPower;
	int _centreBin;
	int _dosleep;
	int _doupper;
};

CTryDecode::CTryDecode(int argc, char **argv) {
	_decoder = new CDecoder(this);
	_fftIn = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * FORWARD_FFT_SIZE);
	_fftOut = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * FORWARD_FFT_SIZE);
	_fftPlan = fftwf_plan_dft_1d(FORWARD_FFT_SIZE, _fftIn, _fftOut, FFTW_FORWARD, FFTW_MEASURE);
	_averageCentreBin = 0.0F;
	_dosleep = 0;
	_doupper = 0;
	for (int a=1; a<argc; a++) {
		if (argv[a][0]=='s')
			_dosleep = 1;
		else if (argv[a][0]=='u')
			_doupper=1;
	}
}

CTryDecode::~CTryDecode(void) {
	fftwf_destroy_plan(_fftPlan);
	fftwf_free(_fftOut);
	fftwf_free(_fftIn);
}

void CTryDecode::go(void) {
	// Grab audio samples from test.wav and convert to complex values (via libsndfile),
	// apply forward FFT, deduce centre frequency then feed to decoder at sane rate.
	SF_INFO sfi = { 0 };
	sf_count_t n;
	SNDFILE *sf = sf_open("test.wav", SFM_READ, &sfi);
	if (!sf) {
		perror("opening test.wav");
		return;
	}
	if (sfi.samplerate != PA_SAMPLE_RATE || sfi.channels != 2) {
		fprintf(stderr, "Unexpected format (not 96kHz 2 channel): 0x%08X\n", sfi.format);
		sf_close(sf);	
		return;
	}
	sf_command(sf, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
	do {
		short buf[2*FORWARD_FFT_SIZE];
		struct timeval ti;
		struct timeval te;
		float maxBin=0.0;
		int binPos = -1;
		gettimeofday(&ti, NULL);
		n = sf_read_short(sf, buf, 2*FORWARD_FFT_SIZE);
		if (n<=0)
			continue;
		for(int i=0; i<FORWARD_FFT_SIZE; i++) {
			_fftIn[i][0] = (float)(buf[2*i]+44)/32768.0;
			_fftIn[i][1] = (float)(buf[2*i+1]-4)/32768.0;
		}
		fftwf_execute(_fftPlan);
		_avePsd[0]=0;
		// NB: FORWARD_FFT_RESULT_SIZE = FORWARD_FFT_SIZE/2 since we ignore negative frequencies in results
		for(int i=0;i<FORWARD_FFT_RESULT_SIZE;i++)
		{
			_psd[i] = sqrt(_fftOut[i][0]*_fftOut[i][0]+_fftOut[i][1]*_fftOut[i][1]);
		}
		int beg = _doupper ? FORWARD_FFT_RESULT_SIZE/2 : 0;
		int end = _doupper ? FORWARD_FFT_RESULT_SIZE : FORWARD_FFT_RESULT_SIZE/2;
		for(int i=beg+75;i<end-75;i++)
		{	
			_avePsd[i]=0;
			for(int j=i-50;j<i+50;j++)
			{
				_avePsd[i] += _psd[j];
			}		
	
			if(maxBin<_avePsd[i])
			{
				maxBin = _avePsd[i];
				binPos = i;
			}
		}
		_centreBin=max(0,min(FORWARD_FFT_RESULT_SIZE-1,_centreBin));
		_averagePeekPower=(PSD_AVERAGE_FACTOR * _avePsd[_centreBin]) + (PSD_INV_AVERAGE_FACTOR * _averagePeekPower);
		if(maxBin>(_averagePeekPower/4)*5 && 0<binPos)
		{
			_averageCentreBin = (CFREQ_AVERAGE_FACTOR * (float)binPos) + (CFREQ_INV_AVERAGE_FACTOR * _averageCentreBin);
			_centreBin = (int)_averageCentreBin+1.0F;
		}
		_decoder->ProcessFFT(_fftOut, _centreBin);
		gettimeofday(&te, NULL);
		int period=(te.tv_sec-ti.tv_sec)*1000000+(te.tv_usec-ti.tv_usec);
		int us=PROCESSING_PERIOD_USEC-period;
		printf("processing took: %d usecs, centreBin=%d\n", period, _centreBin);
		if (_dosleep)
			usleep(us);
	} while (n>0);
	sf_close(sf);
}

void CTryDecode::OnDecodeSuccess(U8* result, U32 length, U32 centreBin) {
	for (int i=0; i<length; i+=16) {
		printf("%02x: ", i);
		for (int j=0; j<16; j++) {
			printf("%02x ", result[i+j]);
		}
		printf("\n");
	}
}

int main(int argc, char **argv) {
	CTryDecode me(argc,argv);
	me.go();
}
