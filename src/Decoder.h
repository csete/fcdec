// FUNcube telemetry decoder test program
// Copyright (c) Phil Ashby, Duncan Hills, Howard Long, 2013
// Released under the terms of the Creative Commons BY-SA-NC
// https://creativecommons.org/licenses/by-nc-sa/3.0/

#pragma once

//#include "DecodeManager.h"
#include "IDecodeResult.h"
#define DOWN_SAMPLE_FILTER_SIZE 27

class CDecoder
{
public:
	CDecoder(IDecodeResult* _listener);
	~CDecoder(void);

	void ProcessFFT(fftwf_complex* fft, ULONG centerBin);
	SAMPLE* AudioBuffer();
	BOOL IsDecoded() { return _isDecoded; }
	//BOOL GetResult(CComSafeArray<byte>* psa) { psa->Add(256, au8Decoded); }

private:
	int dump;
	void RxDownSample(SAMPLE fSample);
	void RxPutNextUCSample(S16 s16UCSamp);

	IDecodeResult* _decodeResultListener;
	FLOAT afSin[RX_SINCOS_SIZE];
	FLOAT afCos[RX_SINCOS_SIZE];
	S32 s32CentreFreq;
	FLOAT fSampleRate;
	FLOAT fVCOPhase;
	int nFIRPos;
	COMPLEXSTRUCT acsHistory[RX_MATCHED_FILTER_SIZE]; // Store a history of samples so that we can apply raised cosine filter
	int nPeakPos;
	int nNewPeakPos;
	int nBitPos;
	FLOAT fBitPhasePos;
	FLOAT afSyncEnergyAvg[SAMPLES_PER_BIT+2];
	COMPLEXSTRUCT csLastIQ;
	FLOAT fEnergyOut;
	U32 u32BitAcc;
	int nBitCount;
	int nByteCount;
	int nDownSampleCount;
	float afHist[DOWN_SAMPLE_FILTER_SIZE];
	int nDownSampleHistIndex;
	U8 au8[5200];
	U8 au8Decoded[BLOCK_SIZE];
	BOOL _isDecoded;
	ULONG _centreBin;
	
	fftwf_complex *_fftInvIn;
	float *_fftInvOut;
	fftwf_plan _fftInvPlan;
};
