## CrossCore Embedded Studio
The [Crosscore Embedded Studio](https://www.analog.com/en/design-center/evaluation-hardware-and-software/software/adswt-cces.html) needs to be installed.
## Where to put Code
Download the [ADSP-BF706_EZ-KIT_Mini-Rel1.1.0 board support package](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-bf706-mini.html#eb-documentation). Open the project AudioFilter_Callback_BF706Mini, and replace AudioCallback.c with this code. You might have to rename FilteredX_LMS.c back to AudioCallback.c. Use the [debugger](https://wiki.analog.com/resources/tools-software/sigmastudiov2/debug_using_crosscore_embedded_studio#:~:text=Select%20Run%E2%86%92Debug%20Configurations,Click%20finish.) to run the project, or use the [command line boot programmer](https://wiki.analog.com/resources/tools-software/crosscore/cces/getting-started/app).

## Fix for Distortion 

The below modification to be applied in "adi_adau1761.c" file located in ADSP-BF706_EZ-KIT_Mini-Rel1.1.0 board support package

<Installation>\Analog Devices\ADSP-BF706_EZ-KIT_Mini-Rel1.1.0\BF706_EZ-Kit_MINI\Blackfin\src\drivers\codec\adau1761
For the two calls of adi_sport_ConfigClock(), replace the forth parameter (true,) in each with:

    #if __CCESVERSION__ >= 0x02040000
	  false, /* CCES-11151 fixed */
	  #else
	  true, /* passing true compensates for CCES-11151 */
	  #endif

This is from an analog devices help forum located [here](https://ez.analog.com/dsp/software-and-development-tools/cces/w/documents/16690/faq-how-to-avoid-audio-distortion-in-bf706mini-bsp-project)

## Functionality
The following code programs the Blackfin BF706 EZLITE to run the filtered-X LMS algorithm. 

Current functionality:
- Runs the FXLMS algorithm
- Allows easy switching between FIR Filters

Needs Improvement:
- The way the samples are called needs to be investigated as they might not be entirely formatted correctly.

Does Not Work:
- No communication between Arduino and Blackfin.
