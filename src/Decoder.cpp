// FUNcube telemetry decoder test program
// Copyright (c) Phil Ashby, Duncan Hills, Howard Long, 2013
// Released under the terms of the Creative Commons BY-SA-NC
// https://creativecommons.org/licenses/by-nc-sa/3.0/

//#include "AudioLib.h"
#include "stdafx.h"
#include <math.h>

#include "Decoder.h"
#include <string.h>

const FLOAT fBitPhaseInc=(FLOAT)(1.0F/SAMPLE_RATE);
const FLOAT fVCOPhaseInc=(FLOAT)(2.0*PI*RX_CARRIER_FREQ/SAMPLE_RATE);
const FLOAT fScaleFFT = (FLOAT)(1.0F / INVERSE_FFT_SIZE);

const int anHalfTable[SAMPLES_PER_BIT] = {4,5,6,7,0,1,2,3};


const FLOAT afFIR[RX_MATCHED_FILTER_SIZE*2]= // Duplicated to make cheap "circular" buffer
{
	-0.0101130691F,-0.0086975143F,-0.0038246093F,+0.0033563764F,+0.0107237026F,+0.0157790936F,+0.0164594107F,+0.0119213911F,
	+0.0030315224F,-0.0076488191F,-0.0164594107F,-0.0197184277F,-0.0150109226F,-0.0023082460F,+0.0154712381F,+0.0327423589F,
	+0.0424493086F,+0.0379940454F,+0.0154712381F,-0.0243701991F,-0.0750320094F,-0.1244834076F,-0.1568500423F,-0.1553748911F,
	-0.1061032953F,-0.0015013786F,+0.1568500423F,+0.3572048240F,+0.5786381191F,+0.7940228249F,+0.9744923010F,+1.0945250059F,
	+1.1366117829F,+1.0945250059F,+0.9744923010F,+0.7940228249F,+0.5786381191F,+0.3572048240F,+0.1568500423F,-0.0015013786F,
	-0.1061032953F,-0.1553748911F,-0.1568500423F,-0.1244834076F,-0.0750320094F,-0.0243701991F,+0.0154712381F,+0.0379940454F,
	+0.0424493086F,+0.0327423589F,+0.0154712381F,-0.0023082460F,-0.0150109226F,-0.0197184277F,-0.0164594107F,-0.0076488191F,
	+0.0030315224F,+0.0119213911F,+0.0164594107F,+0.0157790936F,+0.0107237026F,+0.0033563764F,-0.0038246093F,-0.0086975143F,
	-0.0101130691F,
	-0.0101130691F,-0.0086975143F,-0.0038246093F,+0.0033563764F,+0.0107237026F,+0.0157790936F,+0.0164594107F,+0.0119213911F,
	+0.0030315224F,-0.0076488191F,-0.0164594107F,-0.0197184277F,-0.0150109226F,-0.0023082460F,+0.0154712381F,+0.0327423589F,
	+0.0424493086F,+0.0379940454F,+0.0154712381F,-0.0243701991F,-0.0750320094F,-0.1244834076F,-0.1568500423F,-0.1553748911F,
	-0.1061032953F,-0.0015013786F,+0.1568500423F,+0.3572048240F,+0.5786381191F,+0.7940228249F,+0.9744923010F,+1.0945250059F,
	+1.1366117829F,+1.0945250059F,+0.9744923010F,+0.7940228249F,+0.5786381191F,+0.3572048240F,+0.1568500423F,-0.0015013786F,
	-0.1061032953F,-0.1553748911F,-0.1568500423F,-0.1244834076F,-0.0750320094F,-0.0243701991F,+0.0154712381F,+0.0379940454F,
	+0.0424493086F,+0.0327423589F,+0.0154712381F,-0.0023082460F,-0.0150109226F,-0.0197184277F,-0.0164594107F,-0.0076488191F,
	+0.0030315224F,+0.0119213911F,+0.0164594107F,+0.0157790936F,+0.0107237026F,+0.0033563764F,-0.0038246093F,-0.0086975143F,
	-0.0101130691F
};

const float afLPF[DOWN_SAMPLE_FILTER_SIZE] = 
{
	-6.103515625000e-004F,  /* filter tap #    0 */
	-1.220703125000e-004F,  /* filter tap #    1 */
	+2.380371093750e-003F,  /* filter tap #    2 */
	+6.164550781250e-003F,  /* filter tap #    3 */
	+7.324218750000e-003F,  /* filter tap #    4 */
	+7.629394531250e-004F,  /* filter tap #    5 */
	-1.464843750000e-002F,  /* filter tap #    6 */
	-3.112792968750e-002F,  /* filter tap #    7 */
	-3.225708007813e-002F,  /* filter tap #    8 */
	-1.617431640625e-003F,  /* filter tap #    9 */
	+6.463623046875e-002F,  /* filter tap #   10 */
	+1.502380371094e-001F,  /* filter tap #   11 */
	+2.231445312500e-001F,  /* filter tap #   12 */
	+2.518310546875e-001F,  /* filter tap #   13 */
	+2.231445312500e-001F,  /* filter tap #   14 */
	+1.502380371094e-001F,  /* filter tap #   15 */
	+6.463623046875e-002F,  /* filter tap #   16 */
	-1.617431640625e-003F,  /* filter tap #   17 */
	-3.225708007813e-002F,  /* filter tap #   18 */
	-3.112792968750e-002F,  /* filter tap #   19 */
	-1.464843750000e-002F,  /* filter tap #   20 */
	+7.629394531250e-004F,  /* filter tap #   21 */
	+7.324218750000e-003F,  /* filter tap #   22 */
	+6.164550781250e-003F,  /* filter tap #   23 */
	+2.380371093750e-003F,  /* filter tap #   24 */
	-1.220703125000e-004F,  /* filter tap #   25 */
	-6.103515625000e-004F   /* filter tap #   26 */
};



CDecoder::CDecoder(IDecodeResult* _listener)
{
	dump = 0;
	_decodeResultListener = _listener;
	s32CentreFreq=RX_CARRIER_FREQ;
	fSampleRate=SAMPLE_RATE;
	fVCOPhase=0.0F;
	
	nFIRPos=RX_MATCHED_FILTER_SIZE-1;
	nPeakPos=0;
	nNewPeakPos=0;
	nBitPos=0;
	fBitPhasePos=0.0F;
	csLastIQ.fRe = 0.0F;
	csLastIQ.fIm = 0.0F;	
	fEnergyOut=1.0F;
	u32BitAcc=0;
	nBitCount=0;
	nByteCount=0;
	nDownSampleHistIndex=0;
	nDownSampleCount=0;
	_centreBin = 0;

	_isDecoded = FALSE;

	
	_fftInvIn = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * INVERSE_FFT_SIZE);
	_fftInvOut = (float*) fftwf_malloc(sizeof(float) * INVERSE_FFT_SIZE);
	_fftInvPlan = fftwf_plan_dft_c2r_1d(INVERSE_FFT_SIZE, _fftInvIn, _fftInvOut, FFTW_MEASURE);

	for (int i=0;i<RX_SINCOS_SIZE;i++)
	{
		afSin[i]=(FLOAT)sin(i*2.0*PI/RX_SINCOS_SIZE);
		afCos[i]=(FLOAT)cos(i*2.0*PI/RX_SINCOS_SIZE);
	}

	for (int i=0;i<RX_MATCHED_FILTER_SIZE;i++)
	{
		acsHistory[i].fRe=0.0F;
		acsHistory[i].fIm=0.0F;
	}

	for (int i=0;i<SAMPLES_PER_BIT+2;i++)
	{
		afSyncEnergyAvg[i]=0.0F;
	}
}


CDecoder::~CDecoder(void)
{
	fftwf_free(_fftInvIn);
	fftwf_free(_fftInvOut);
}


void CDecoder::ProcessFFT(fftwf_complex* fft, ULONG centreBin)
{
	_centreBin = centreBin;
	_isDecoded = FALSE;
	
	// clear the buffer
	memset(_fftInvIn, 0, INVERSE_FFT_SIZE*sizeof(fftwf_complex));
	// copy 204 buckets to the inverse transform, in the right place		
	//memcpy(_fftInvIn, _fftOut, INVERSE_FFT_SIZE*sizeof(fftwf_complex));
	memcpy(_fftInvIn, &fft[centreBin-102], (204*sizeof(fftwf_complex)));

	fftwf_execute(_fftInvPlan);
	for(ULONG i=0;i<INVERSE_FFT_SIZE;i++)
	{
		_fftInvOut[i]*=fScaleFFT;
		RxDownSample((float)_fftInvOut[i]);
	}
	dump = 0;
}

SAMPLE* CDecoder::AudioBuffer()
{
	//ATLASSERT(INVERSE_FFT_SIZE==FRAMES_PER_BUFFER);

	return (SAMPLE*)_fftInvOut;

	//if(NULL!= resultBuffer)
	//{
	//	memcpy(resultBuffer,_fftInvOut,FRAMES_PER_BUFFER*sizeof(SAMPLE));
	//}
}

void CDecoder::RxDownSample(SAMPLE fSample) // Downsample from 96000 to 9600SPS
{
	afHist[nDownSampleHistIndex]=fSample;

	// Only need to produce a filtered result every now and again
	if (++nDownSampleCount>=PA_SAMPLE_RATE/SAMPLE_RATE)
	{
		nDownSampleCount=0;

		const float *pfLPF=afLPF;
		float fSum=0.0f;

		for (int i=nDownSampleHistIndex;i<DOWN_SAMPLE_FILTER_SIZE;i++)
		{
			fSum+=afHist[i] * (*pfLPF++);
		}
		for (int i=0;i<nDownSampleHistIndex;i++)
		{
			fSum+=afHist[i] * (*pfLPF++);
		}

		RxPutNextUCSample((S16)(fSum*0.9*32767));
	}

	if (++nDownSampleHistIndex>=DOWN_SAMPLE_FILTER_SIZE)
	{
		nDownSampleHistIndex=0;
	}
}

void CDecoder::RxPutNextUCSample(S16 s16UCSamp)
{
	// Takes upconverted samples
	
	FLOAT fRe=0.0F;
	FLOAT fIm=0.0F;
	const FLOAT *pfFIR=afFIR+RX_MATCHED_FILTER_SIZE-nFIRPos;
	COMPLEXSTRUCT *pcsHistory=acsHistory;
	int i;

	if (dump) fprintf(stderr, "vcophase=%f sample=%d nfirpos=%d\n", fVCOPhase, s16UCSamp, nFIRPos);
	// Complex downconversion...
	fVCOPhase+=fVCOPhaseInc;
	if (fVCOPhase>=2.0F*PI)
	{
		fVCOPhase=(FLOAT)(fVCOPhase-2.0F*PI);
	}
	acsHistory[nFIRPos].fRe=(FLOAT)(s16UCSamp*afCos[(int)(fVCOPhase*(RX_SINCOS_SIZE/(2.0F*PI)))]);
	acsHistory[nFIRPos].fIm=(FLOAT)(s16UCSamp*afSin[(int)(fVCOPhase*(RX_SINCOS_SIZE/(2.0F*PI)))]);

	// Apply Root raised cosine FIR
	for (i=0;i<RX_MATCHED_FILTER_SIZE;i++)
	{
		fRe+=pcsHistory->fRe*(*pfFIR);
		fIm+=pcsHistory->fIm*(*pfFIR);
		pfFIR++;
		pcsHistory++;
	}
	nFIRPos--;
	if (nFIRPos<0)
	{
		nFIRPos=RX_MATCHED_FILTER_SIZE-1;
	}
	if (dump) fprintf(stderr, "fRe=%f fIm=%f\n", fRe, fIm);

	// Bit clock detection
	{
		const FLOAT fEnergySmooth=(1.0F/200.0F);
		FLOAT fEnergy=fRe*fRe+fIm*fIm;

		afSyncEnergyAvg[nBitPos]=(FLOAT)((1.0F-fEnergySmooth)*afSyncEnergyAvg[nBitPos]+fEnergySmooth*fEnergy);
		if (dump) fprintf(stderr, "bitPos=%d peakPos=%d energy1=%f\n", nBitPos, nPeakPos, fEnergy);
		if (nBitPos==nPeakPos)
		{
			COMPLEXSTRUCT csVect;
			FLOAT fEnergy2;
			const FLOAT fEnergySmooth2=(1.0F/800.0F);

			// Now to decode the symbol...
			fEnergyOut=(FLOAT)((1.0F-fEnergySmooth2)*fEnergyOut+fEnergySmooth2*fEnergy);

			csVect.fRe=-(csLastIQ.fRe*fRe+csLastIQ.fIm*fIm); // DBPSK!
			csVect.fIm=(csLastIQ.fRe*fIm-csLastIQ.fIm*fRe); // For use in AFC
			csLastIQ.fRe=fRe;
			csLastIQ.fIm=fIm;

			fEnergy2=(FLOAT)sqrt(csVect.fRe*csVect.fRe+csVect.fIm*csVect.fIm);
			if (fEnergy2>100.0F)
			{
				// Insert CalcPhaseError + other stuff here
				BOOL bBit=csVect.fRe<0.0F;

				//{ // Prepare global variable to display phasor
				//	_csPhaseDisplay.fRe=csVect.fRe/fEnergy2;
				//	_csPhaseDisplay.fIm=csVect.fIm/fEnergy2;
				//}

				{
					// Alternative correlator method
					static S8 as8FECBits[5200];
					static const S8 as8SyncVector[65]={1,1,1,1,1,1,1,-1,-1,-1,-1,1,1,1,-1,1,1,1,1,-1,-1,1,-1,1,1,-1,-1,1,-1,-1,1,-1,-1,-1,-1,-1,-1,1,-1,-1,-1,1,-1,-1,1,1,-1,-1,-1,1,-1,1,1,1,-1,1,-1,1,1,-1,1,1,-1,-1,-1};
					int i;
					int nSum=0;

					memmove(as8FECBits,as8FECBits+1,5200-1);
					as8FECBits[5200-1]=(bBit ? 1 : -1);

					for (i=0;i<65;i++)
					{
						nSum+=as8FECBits[i*80]*as8SyncVector[i];						
					}
					if(nSum>=25)
					{
						//char szOut[32];							
						//sprintf(szOut,"Sum=%d, Phase=%f\r\n",nSum,fBitPhasePos);
						//OutputDebugStringA(szOut);
						fprintf(stderr, "Sum=%d, Phase=%f\n",nSum,fBitPhasePos);
					}
					if (nSum>=45)
					{
						for (i=0;i<5200;i++)
						{
							au8[i]=((as8FECBits[i]==1) ? 0xC0 : 0x40);
						}
						fprintf(stderr, "Attempting FECDecode\n");
						if (FECDecode(au8,au8Decoded))
						{
							// Indicate data ready for collection
							_isDecoded = TRUE;
							if(NULL!=_decodeResultListener)
							{
								_decodeResultListener->OnDecodeSuccess(au8Decoded, sizeof(au8Decoded), _centreBin);
							}

							// send data to clients
							//CComSafeArray<byte> sa(0UL);
							//sa.Add(256,au8Decoded);
							//Fire_DataReceived(CComVariant(sa));
							fprintf(stderr, "FECDecode OK!\n");
						}
						else
						{
							// Error decoding, send an empty data packet.
							//CComSafeArray<byte> sa(0UL);							
							//Fire_DataReceived(CComVariant(sa));
							fprintf(stderr, "FECDecode FAIL\n");
						}
					}
				}
			}
		}
		if (nBitPos==anHalfTable[nPeakPos]) // Detect if half way into next bit
		{
			nPeakPos=nNewPeakPos; // if so, this is the least disruptive time to reset
		}
		nBitPos++;
		if (nBitPos>=SAMPLES_PER_BIT)
		{
			nBitPos=0;
		}
		fBitPhasePos+=fBitPhaseInc;
		if (fBitPhasePos>=BIT_TIME) // Each bit time...
		{
			FLOAT fMax=-1.0e10F;

			fBitPhasePos=(FLOAT)(fBitPhasePos-BIT_TIME);
			nBitPos=0;

			for (i=0;i<SAMPLES_PER_BIT;i++) // Find max energy over past samples
			{
				fEnergy=afSyncEnergyAvg[i];
				if (fEnergy>fMax)
				{
					nNewPeakPos=i;
					fMax=fEnergy;
				}
			}
		}
	}
}
