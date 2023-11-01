/*********************************************************************************

Copyright(c) 2015 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

*********************************************************************************/

/*
 *
 * This example demonstrates how to use the ADAU1761 codec driver to receive audio
 * samples from the Line Input and transmit those audio samples to the headphone
 * output (talkthrough).
 *
 * On the ADSP-BF706 EZ-KIT Mini™:
 * Connect an audio source to the LINE IN jack (J1)
 * Connect headphones to the HP jack (J2).
 */

#include <drivers/codec/adau1761/adi_adau1761.h>
//#include <drivers/spi/adi_spi.h> //---
#include <services/pwr/adi_pwr.h>
#include <stdio.h>
#include <string.h>
#include <filter.h> //---
#include <fract.h> //--
#include <stdfix.h> //--
#include <math_const.h> //--



/* For SPI
#if defined(__ADSPGCC__)
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
*/


#define SAMPLES_PER_CHAN   1u    //--
#define Delta 0.1r


/* SigmaStudio exported file */
#include "SigmaStudio\export\export_IC_1.h"

#include "TalkThrough_BF706Mini.h"

/* ADI initialization header */
#include "system/adi_initialize.h"

/* User Macro - select sample rate */
#define SAMPLE_RATE  (ADI_ADAU1761_SAMPLE_RATE_48KHZ)

/* User Macro - select the number of audio samples (stereo) for each buffer */
//#define NUM_AUDIO_SAMPLES       64u  /* 32 left + 32 right */
#define NUM_AUDIO_SAMPLES       2u  /* 1 left + 1 right */
#define NUM_COEFF				500u 	//---

/* 32-bits per sample (24-bit audio) */
#define BUFFER_SIZE      (NUM_AUDIO_SAMPLES*sizeof(uint32_t))

/* used for exit timeout */
#define MAXCOUNT (500000u)

/*
 * SPORT device memory
 */
#pragma align(4)
static uint8_t sportRxMem[ADI_SPORT_DMA_MEMORY_SIZE];

#pragma align(4)
static uint8_t sportTxMem[ADI_SPORT_DMA_MEMORY_SIZE];

/*
 * Audio buffers
 */
#pragma align(4)
static uint32_t RxBuffer1[NUM_AUDIO_SAMPLES];

#pragma align(4)
static uint32_t RxBuffer2[NUM_AUDIO_SAMPLES];

#pragma align(4)
static uint32_t TxBuffer1[NUM_AUDIO_SAMPLES];

#pragma align(4)
static uint32_t TxBuffer2[NUM_AUDIO_SAMPLES];

/* audio buffer pointers */
static uint32_t *pRxAudioBuffer;
static uint32_t *pTxAudioBuffer;

/* SPORT info struct */
static ADI_ADAU1761_SPORT_INFO sportRxInfo;
static ADI_ADAU1761_SPORT_INFO sportTxInfo;

typedef enum
{
	NONE,
	START_TALKTHROUGH,
	TALKTHROUGH,
	STOP_TALKTHROUGH
} MODE;

static MODE eMode = NONE;

/* Memory required for codec driver - must add PROGRAM_SIZE_IC_1 size for data transfer to codec */
static uint8_t codecMem[ADI_ADAU1761_MEMORY_SIZE + PROGRAM_SIZE_IC_1];

/* Memory required for SPI driver */
//static uint8_t SPIDriverMemory[ADI_SPI_INT_MEMORY_SIZE];

/* adau1761 device handle */
static ADI_ADAU1761_HANDLE  hADAU1761;

/* SPI driver handle */
//static ADI_SPI_HANDLE hDevice;


static bool bError;
static volatile uint32_t count;

/* error check */
static void CheckResult(ADI_ADAU1761_RESULT result)
{
	if (result != ADI_ADAU1761_SUCCESS) {
		printf("Codec failure\n");
		bError = true;
	}
}


/* SPI error check */
/*
static void SPI_CheckResult(ADI_SPI_RESULT result)
{
	if (result != ADI_SPI_SUCCESS) {
		printf("SPI failure\n");
		bError = true;
	}
}*/


/* codec record mixer */
static void MixerEnable(bool bEnable)
{
	ADI_ADAU1761_RESULT result;
	uint8_t value;

	if (bEnable)
	{
		/* enable the record mixer (left) */
		result = adi_adau1761_SetRegister (hADAU1761, REC_MIX_LEFT_REG, 0x0B); /* LINP mute, LINN 0 dB */
		CheckResult(result);

		/* enable the record mixer (right) */
		result = adi_adau1761_SetRegister (hADAU1761, REC_MIX_RIGHT_REG, 0x0B);  /* RINP mute, RINN 0 dB */
		CheckResult(result);
	}
	else
	{
		/* disable the record mixer (left) */
		result = adi_adau1761_GetRegister (hADAU1761, REC_MIX_LEFT_REG, &value);
		result = adi_adau1761_SetRegister (hADAU1761, REC_MIX_LEFT_REG, value & 0xFE);
		CheckResult(result);

		/* disable the record mixer (right) */
		result = adi_adau1761_GetRegister (hADAU1761, REC_MIX_RIGHT_REG, &value);
		result = adi_adau1761_SetRegister (hADAU1761, REC_MIX_RIGHT_REG, value & 0xFE);
		CheckResult(result);
	}
}

/* codec driver configuration */
static void CodecSetup(void)
{
	ADI_ADAU1761_RESULT result;

	/* Open the codec driver */
	result = adi_adau1761_Open(ADAU1761_DEV_NUM,
			codecMem,
			sizeof(codecMem),
			ADI_ADAU1761_COMM_DEV_TWI,
			&hADAU1761);
	CheckResult(result);

	/* Configure TWI - BF706 EZ-Kit MINI version 1.0 uses TWI */
	result = adi_adau1761_ConfigTWI(hADAU1761, TWI_DEV_NUM, ADI_ADAU1761_TWI_ADDR0);
	CheckResult(result);

	/* load Sigma Studio program exported from *_IC_1.h */
	result = adi_adau1761_SigmaStudioLoad(hADAU1761, default_download_IC_1);
	CheckResult(result);

	/* config SPORT for Rx data transfer */
	sportRxInfo.nDeviceNum = SPORT_RX_DEVICE;
	sportRxInfo.eChannel = ADI_HALF_SPORT_B;
	sportRxInfo.eMode = ADI_ADAU1761_SPORT_I2S;
	sportRxInfo.hDevice = NULL;
	sportRxInfo.pMemory = sportRxMem;
	sportRxInfo.bEnableDMA = true;
	sportRxInfo.eDataLen = ADI_ADAU1761_SPORT_DATA_24;
	sportRxInfo.bEnableStreaming = true;

	result = adi_adau1761_ConfigSPORT (hADAU1761,
			ADI_ADAU1761_SPORT_INPUT, &sportRxInfo);
	CheckResult(result);

	/* config SPORT for Tx data transfer */
	sportTxInfo.nDeviceNum = SPORT_TX_DEVICE;
	sportTxInfo.eChannel = ADI_HALF_SPORT_A;
	sportTxInfo.eMode = ADI_ADAU1761_SPORT_I2S;
	sportTxInfo.hDevice = NULL;
	sportTxInfo.pMemory = sportTxMem;
	sportTxInfo.bEnableDMA = true;
	sportTxInfo.eDataLen = ADI_ADAU1761_SPORT_DATA_24;
	sportTxInfo.bEnableStreaming = true;

	result = adi_adau1761_ConfigSPORT (hADAU1761,
			ADI_ADAU1761_SPORT_OUTPUT, &sportTxInfo);
	CheckResult(result);

	result = adi_adau1761_SelectInputSource(hADAU1761, ADI_ADAU1761_INPUT_ADC);
	CheckResult(result);

	/* disable mixer */
	MixerEnable(false);

	result = adi_adau1761_SetVolume (hADAU1761,
			ADI_ADAU1761_VOL_HEADPHONE,
			ADI_ADAU1761_VOL_CHAN_BOTH,
			true,
			57); /* 0 dB */
	CheckResult(result);

	result = adi_adau1761_SetSampleRate (hADAU1761, SAMPLE_RATE);
	CheckResult(result);
}

/*
static void SPISetup(void){
	ADI_SPI_RESULT SPIresult;

	// Open SPI Driver
	SPIresult = adi_spi_Open(1,
			SPIDriverMemory,
			(uint32_t)ADI_SPI_DMA_MEMORY_SIZE,
			&hDevice);
	SPI_CheckResult(SPIresult);

//	 adi_spi_SetMaster(&hDevice, true);
//	 adi_spi_SetHwSlaveSelect(&hDevice, true);
//	 adi_spi_SetSlaveSelect(&hDevice, false);
//	 adi_spi_SetWordSize(&hDevice, ADI_SPI_TRANSFER_8BIT);
//	 adi_spi_SetClock(&hDevice, );


    // No callbacks required
	SPIresult = adi_spi_RegisterCallback(hDevice, NULL, NULL);
	SPI_CheckResult(SPIresult);

    // Disable DMA
	SPIresult = adi_spi_EnableDmaMode(hDevice, false);
	SPI_CheckResult(SPIresult);
}*/

int main(void)
{

	/* Input and Output, Delay Line Declarations */
	fract *inLeft;
	fract *inRight;

	fract delayInput[NUM_COEFF] = {0.0r}; //or SAMPLES_PER_CHAN
	fract delayError[NUM_COEFF] = {0.0r};

	fract outLeft = 0.0r;
	fract outRight = 0.0r;

	fract plant1_coeffs[NUM_COEFF] = {0.0r};
	fract plant2_coeffs[NUM_COEFF] = {0.5r};
	fract control_coeffs[NUM_COEFF] = {0.0r};

	plant1_coeffs[0] = FRACT_MAX;

	/* Filter States */
	_fir_fx16_state statePlant1;
	_fir_fx16_state stateControl;


	/* Indexing variables */
	int i = 0;
	int n = 0;
	int filterIdx = 1;

	bool bExit;
	bool bRxAvailable;
	bool bTxAvailable;
//	bool bSPIAvailable = false; // flag indicating the is a SPI transceiver available
	ADI_ADAU1761_RESULT result;
	ADI_PWR_RESULT pwrResult;
//	ADI_SPI_RESULT SPIresult;

	/* pointer to store transceiver */
//	ADI_SPI_TRANSCEIVER *pTrans;
	/* transceiver buffers */
	//uint8_t Prologue[4]  = {0x00, 0x01, 0x02, 0x03};
//	uint8_t TxSPIBuffer[1]  = {0x01};
//	uint8_t RxSPIBuffer[1];
	/* transceiver configurations */
	//ADI_SPI_TRANSCEIVER Transceiver1  = {&Prologue[0], 4, &TxBuffer[0], 8, &RxBuffer[0], 8};
//	ADI_SPI_TRANSCEIVER Transceiver1  = {NULL, 0, &TxSPIBuffer[0], 1, &RxSPIBuffer[0], 1};
//	ADI_SPI_TRANSCEIVER Transceiver2  = {NULL, 0, &TxSPIBuffer[0], 1, NULL, 0};


	bExit = false;

	/* setup processor mode and frequency */
	pwrResult = adi_pwr_Init(0, CLKIN);
	pwrResult = adi_pwr_SetPowerMode(0,	ADI_PWR_MODE_FULL_ON);
	pwrResult = adi_pwr_SetClkControlRegister(0, ADI_PWR_CLK_CTL_MSEL, MSEL);
	pwrResult = adi_pwr_SetClkDivideRegister(0, ADI_PWR_CLK_DIV_CSEL, CSEL);
	pwrResult = adi_pwr_SetClkDivideRegister(0, ADI_PWR_CLK_DIV_SYSSEL, SYSSEL);

    adi_initComponents(); /* auto-generated code */

    /* configure the codec driver */
    CodecSetup();

    /* configure the SPI driver */
//    SPISetup();

    /* Configure filter state */
    if(filterIdx == 1){
    	fir_init (statePlant1, plant1_coeffs, delayInput, NUM_COEFF,0);
    	/*
    } else if (filterIdx == 2){
    	fir_init (statePlant, plant2_coeffs, delayInput, NUM_COEFF,0); */
    } else {
    	printf("Bad Filter index\n");
    }

    fir_init (stateControl, control_coeffs, delayError, NUM_COEFF,0);

    /* Zero delay line to start or reset the filter
    for (i = 0; i < NUM_COEFF; i++)
    {
    	   delayInput[i] = 0.0r;
    	   delayError[i] = 0.0r;
    }*/

	count = 0u;
	eMode = START_TALKTHROUGH;


	while(!bExit && !bError)
	{
		switch(eMode)
		{
			case START_TALKTHROUGH:

				/* stop current input */
				result = adi_adau1761_EnableInput (hADAU1761, false);
				CheckResult(result);

				/* stop current output */
				result = adi_adau1761_EnableOutput (hADAU1761, false);
				CheckResult(result);

				/* submit Rx buffer1 */
				result = adi_adau1761_SubmitRxBuffer(hADAU1761, RxBuffer1, BUFFER_SIZE);
				CheckResult(result);

				/* submit Rx buffer2 */
				result = adi_adau1761_SubmitRxBuffer(hADAU1761,	RxBuffer2, BUFFER_SIZE);
				CheckResult(result);

				/* submit Tx buffer1 */
				result = adi_adau1761_SubmitTxBuffer(hADAU1761, TxBuffer1, BUFFER_SIZE);
				CheckResult(result);

				/* submit Tx buffer2 */
				result = adi_adau1761_SubmitTxBuffer(hADAU1761,	TxBuffer2, BUFFER_SIZE);
				CheckResult(result);

				result = adi_adau1761_EnableInput (hADAU1761, true);
				CheckResult(result);

				result = adi_adau1761_EnableOutput (hADAU1761, true);
				CheckResult(result);

				/* enable record mixer */
				MixerEnable(true);

				i = 0u;

				eMode = TALKTHROUGH;
				break;
			case TALKTHROUGH:
				result = adi_adau1761_GetRxBuffer (hADAU1761, (void**)&pRxAudioBuffer);
				CheckResult(result);

				if (pRxAudioBuffer != NULL)
				{
					/* re-submit buffer */
					result = adi_adau1761_SubmitRxBuffer(hADAU1761, pRxAudioBuffer, BUFFER_SIZE);
					CheckResult(result);

					/* get an available Tx buffer */
					result = adi_adau1761_GetTxBuffer (hADAU1761, (void**)&pTxAudioBuffer);
					CheckResult(result);

					if (pTxAudioBuffer != NULL)
					{
						/*
						 * Insert audio algorithm here
						 */
						inLeft = (fract*)(*(pRxAudioBuffer)>>8);
						inRight = (fract*)(*(pRxAudioBuffer+1)>>8);

//						TxSPIBuffer[0] += 2;
//						TxSPIBuffer[0] = TxSPIBuffer[0] % 100;

						/*
						 // submit the SPI transceiver's buffers
							SPIresult = adi_spi_SubmitBuffer(hDevice, &Transceiver1);
							SPI_CheckResult(SPIresult);
							SPIresult = adi_spi_SubmitBuffer(hDevice, &Transceiver2);
							SPI_CheckResult(SPIresult);


						//wait here until a transceiver is available

							while(bSPIAvailable == false)
							{
								SPIresult = adi_spi_IsBufferAvailable(hDevice, &bSPIAvailable);
								SPI_CheckResult(SPIresult);
							}


							// fetch the transceiver buffer just processed
							SPIresult = adi_spi_GetBuffer(hDevice, &pTrans);
							SPI_CheckResult(SPIresult);


					     //submit the SPI transceiver's buffers
						//SPIresult = adi_spi_ReadWrite(hDevice, &Transceiver1);
						//SPIresult = adi_spi_ReadWrite(hDevice, &Transceiver2);

						*/

						/* Estimate channel filter */
							fir_fx16 (inLeft, &outLeft, SAMPLES_PER_CHAN, &statePlant1);

							for(n=0;n<NUM_COEFF;n++) {
								control_coeffs[n] = control_coeffs[n] + Delta * (*inRight) * outLeft;
							}


							fir_fx16 (inRight, &outRight, SAMPLES_PER_CHAN, &stateControl);

						*(pTxAudioBuffer) = bitslr((long fract)(outRight<<8));
						*(pTxAudioBuffer+1) = bitslr((long fract)(outRight<<8));

						/* talkthrough - copy the audio data from Rx to Tx */
						//memcpy(&pTxAudioBuffer[0], &pRxAudioBuffer[0], BUFFER_SIZE);

						/*
						 * End audio algorithm
						 */

						/* re-submit buffer */
						result = adi_adau1761_SubmitTxBuffer(hADAU1761, pTxAudioBuffer, BUFFER_SIZE);
						CheckResult(result);
					}

				}
				break;
			case STOP_TALKTHROUGH:
				result = adi_adau1761_EnableInput (hADAU1761, false);
				CheckResult(result);

				result = adi_adau1761_EnableOutput (hADAU1761, false);
				CheckResult(result);

				eMode = NONE;
				bExit = true;
				break;

			default:
				break;
		}

		/* exit the loop after a while */
		if (count > MAXCOUNT)
		{
			eMode = STOP_TALKTHROUGH;
		}
		count++;

	}


	/* close the SPI driver */
//	SPIresult = adi_spi_Close(hDevice);
//	SPI_CheckResult(SPIresult);

	result = adi_adau1761_Close(hADAU1761);
	CheckResult(result);

	if (!bError) {
		printf("All done\n");
	} else {
		printf("Audio error\n");
	}

	return 0;
}
