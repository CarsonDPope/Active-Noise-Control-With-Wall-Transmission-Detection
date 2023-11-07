/*********************************************************************************

Copyright(c) 2015 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

*********************************************************************************/

/*
 * This example demonstrates how to use the ADAU1761 codec driver to receive audio
 * samples from the Line Input, apply a audio processing algorithm then transmit
 * the processed data to the Headphone (HP) Output.
 *
 * On the ADSP-BF706 EZ-KIT Mini™:
 * Connect an audio source to the LINE IN jack (J1)
 * Connect headphones to the HP jack (J2).
 */

#include <drivers/codec/adau1761/adi_adau1761.h>
#include <services/pwr/adi_pwr.h>
#include <services/gpio/adi_gpio.h>
#include <stdio.h>
#include <string.h>

#include <stdfix.h> //--
#include <fract.h> //--
#include <math_const.h> //--

/* SigmaStudio exported file */
#include "SigmaStudio\export\export_IC_1.h"

#include "AudioCallback.h"

/* ADI initialization header */
#include "system/adi_initialize.h"

/* User Macro - select sample rate */
#define SAMPLE_RATE  (ADI_ADAU1761_SAMPLE_RATE_48KHZ)

/* User Macro - select the number of audio samples (stereo) for each buffer */
//#define NUM_AUDIO_SAMPLES       64u  /* 32 left + 32 right */
#define NUM_AUDIO_SAMPLES       2u  /* 1 left + 1 right */

#define NUM_COEFF 100u
#define Delta 	  0.001r

/* 32-bits per sample (24-bit audio) */
#define BUFFER_SIZE      (NUM_AUDIO_SAMPLES*sizeof(uint32_t))

/* used for exit timeout */
#define MAXCOUNT (0x100000000L)

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

//#pragma align(4)
static uint32_t filterOutput[NUM_AUDIO_SAMPLES];

static uint8_t *pRxBuffer;
static uint8_t *pTxBuffer;

/* SPORT info struct */
static ADI_ADAU1761_SPORT_INFO sportRxInfo;
static ADI_ADAU1761_SPORT_INFO sportTxInfo;

/* Memory required for codec driver - must add PROGRAM_SIZE_IC_1 size for data transfer to codec */
static uint8_t codecMem[ADI_ADAU1761_MEMORY_SIZE + PROGRAM_SIZE_IC_1];

/* adau1761 device handle */
static ADI_ADAU1761_HANDLE  hADAU1761;

static FILTER_MODE mode = NO_FILTER;

static bool bError = false;

static volatile uint64_t count = 0;

static fract plant1_coeffs[NUM_COEFF] = {FRACT_MAX};

/* error check */
static void CheckResult(ADI_ADAU1761_RESULT result)
{
	if (result != ADI_ADAU1761_SUCCESS) {
		printf("Codec failure\n");
		bError = true;
	}
}

/* codec callback */
static void ADAU1761Callback(void *pCBParam, uint32_t Event, void *pArg)
{
	ADI_ADAU1761_RESULT result;

	static fract delayInput[NUM_COEFF] = {0.0r};
	static fract delayError[NUM_COEFF] = {0.0r};

	static fract control_coeffs[NUM_COEFF] = {0.0r};

	static int i;
	static int j;

	static accum sum;

    switch(Event)
    {
    	case (uint32_t)ADI_ADAU1761_EVENT_RX_BUFFER_PROCESSED:
   			pRxBuffer = pArg;

			/* re-sumbit the buffer for processing */
	    	result = adi_adau1761_SubmitRxBuffer(hADAU1761, pRxBuffer, BUFFER_SIZE);
			CheckResult(result);

			if (mode != NO_FILTER)
			{
				/* filter the signal - audio processing */
				//AudioFilter((fract32*)pRxBuffer, (fract32*)&filterOutput[0]);

				delayInput[0] = rbits((short)((pRxBuffer[0])<<8));
				delayError[0] = rbits((short)((pRxBuffer[1])<<8));

				/* Estimate channel filter */

			    // Convolution
			    sum = 0.0k;
			    for(i=0; i<NUM_COEFF; i++){
			        sum += delayInput[i]*plant1_coeffs[i];
			    } // Convolution loop

			    // shift delay line
			    for(j=NUM_COEFF-1;j>0;j--){
					delayInput[j]=delayInput[j-1];
				} // shift loop

			    // Clipping
			    if(sum > FRACT_MAX){
			        sum = FRACT_MAX;
			    } else if (sum < FRACT_MIN){
			        sum = FRACT_MIN;
			    } // Clipping if statements

				for(i=0;i<NUM_COEFF;i++) {
					control_coeffs[i] = control_coeffs[i] + Delta * (delayError[i]) * (fract)sum; //or long fract if doesnt work
				} // LMS

			    // Convolution
			    sum = 0.0k;
			    for(i=0; i<NUM_COEFF; i++){
			        sum += delayError[i]*control_coeffs[i];
			    } // Convolution loop

			    // shift delay line
			    for(j=NUM_COEFF-1;j>0;j--){
					delayError[j]=delayError[j-1];
				} // shift loop

			    // Clipping
			    if(sum > FRACT_MAX){
			        sum = FRACT_MAX;
			    } else if (sum < FRACT_MIN){
			        sum = FRACT_MIN;
			    } // Clipping if statements

			    filterOutput[0] = bitslr((long fract)(sum)>>8);
			    filterOutput[1] = bitslr((long fract)(sum)>>8);
			}
            break;

    	case (uint32_t)ADI_ADAU1761_EVENT_TX_BUFFER_PROCESSED:
       		pTxBuffer = pArg;

    	    /*
    	     * transmit the audio
    	     */
    		if ((pRxBuffer != NULL) && (pTxBuffer != NULL))
    		{
    			if (mode == NO_FILTER)
    			{
    				/* no filtering - copy the unfiltered audio data from Rx to Tx */
    				memcpy(&pTxBuffer[0], &pRxBuffer[0], BUFFER_SIZE);
    			}
    			else
    			{
					/* copy the filtered audio data from Rx to Tx */
					memcpy(&pTxBuffer[0], &filterOutput[0], BUFFER_SIZE);
    			}
    		}

       		/* re-sumbit the buffer for processing */
	    	result = adi_adau1761_SubmitTxBuffer(hADAU1761, pTxBuffer, BUFFER_SIZE);
			CheckResult(result);

            break;

    	default:
            break;
    }
}

/* use the push button to set the filter type */
void gpioCallback(ADI_GPIO_PIN_INTERRUPT ePinInt, uint32_t Data, void *pCBParam)
{
    if (ePinInt == PUSH_BUTTON1_PINT)
    {
        if (Data & PUSH_BUTTON1_PIN)
        {
            /* push button 1 */
        	if (mode == NO_FILTER)
        	{
        		mode = LOW_PASS;

        		/* change the filter type */
//				FilterInit(mode);
        	}
        	else if (mode == LOW_PASS)
        	{
        		mode = HIGH_PASS;

				/* change the filter type */
//				FilterInit(mode);
        	}
        	else
        	{
        		mode = NO_FILTER;
        	}
        }
    }
}

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
			57);  /* 0 dB */
	CheckResult(result);

	result = adi_adau1761_SetSampleRate (hADAU1761, SAMPLE_RATE);
	CheckResult(result);
}

/* push button setup */
static void GpioSetup(void)
{
	uint32_t gpioMaxCallbacks;
	ADI_GPIO_RESULT gpioResult;
	static uint8_t gpioMemory[ADI_GPIO_CALLBACK_MEM_SIZE];

    /* initialize the GPIO service */
	gpioResult = adi_gpio_Init(
			(void*)gpioMemory,
			ADI_GPIO_CALLBACK_MEM_SIZE,
			&gpioMaxCallbacks);

	/*
	 * Setup Push Button 1
	 */

	/* set GPIO input */
	gpioResult = adi_gpio_SetDirection(
		PUSH_BUTTON1_PORT,
		PUSH_BUTTON1_PIN,
	    ADI_GPIO_DIRECTION_INPUT);

    /* set input edge sense */
	gpioResult = adi_gpio_SetPinIntEdgeSense(
		PUSH_BUTTON1_PINT,
		PUSH_BUTTON1_PIN,
	    ADI_GPIO_SENSE_RISING_EDGE);

    /* register gpio callback */
    gpioResult = adi_gpio_RegisterCallback(
    	PUSH_BUTTON1_PINT,
    	PUSH_BUTTON1_PIN,
    	gpioCallback,
   	    (void*)0);

	/* enable interrupt mask */
    gpioResult = adi_gpio_EnablePinInterruptMask(
		PUSH_BUTTON1_PINT,
		PUSH_BUTTON1_PIN,
	    true);
}

int main(void)
{
	ADI_ADAU1761_RESULT result;
	ADI_PWR_RESULT pwrResult;

	/* setup processor mode and frequency */
	pwrResult = adi_pwr_Init(0, CLKIN);
	pwrResult = adi_pwr_SetPowerMode(0,	ADI_PWR_MODE_FULL_ON);
	pwrResult = adi_pwr_SetClkControlRegister(0, ADI_PWR_CLK_CTL_MSEL, MSEL);
	pwrResult = adi_pwr_SetClkDivideRegister(0, ADI_PWR_CLK_DIV_CSEL, CSEL);
	pwrResult = adi_pwr_SetClkDivideRegister(0, ADI_PWR_CLK_DIV_SYSSEL, SYSSEL);

    adi_initComponents(); /* auto-generated code */

    printf("\n\nPress push button 1 (PB1) on the ADSP-BF706 EZ-Kit Mini to cycle the filter types (no filter, low pass, high pass)\n");

    /* push button setup */
    GpioSetup();

    /* configure the codec driver */
    CodecSetup();

	/* register a Rx callback */
	result = adi_adau1761_RegisterRxCallback (hADAU1761,
			ADAU1761Callback, NULL);
	CheckResult(result);

	/* register a Tx callback */
	result = adi_adau1761_RegisterTxCallback (hADAU1761,
			ADAU1761Callback, NULL);
	CheckResult(result);

	/*
	 * submit buffers and enable audio
	 */

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

	/*
	 * process audio in the callback
	 */

	while(bError == false)
	{
//		count++;
		/* exit the loop after a few minutes */
//		if (count > MAXCOUNT)
//		{
//			break;
//		}
	}

	/* disable audio */
	result = adi_adau1761_EnableInput (hADAU1761, false);
	CheckResult(result);

	result = adi_adau1761_EnableOutput (hADAU1761, false);
	CheckResult(result);

	result = adi_adau1761_Close(hADAU1761);
	CheckResult(result);

	if (!bError) {
		printf("All done\n");
	} else {
		printf("Audio error\n");
	}

	return 0;
}
