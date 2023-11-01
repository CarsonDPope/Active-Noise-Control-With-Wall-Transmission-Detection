Blackfin Code

This section needs to explain the ide, how to download and open the talkthrough program, the fix involving adau code driver

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

--------

also needs to explain which file in talkthrough program to replace with files in here

how to load on to blackfin ( especially for non debugging mode )

also needs to explain function, version, and stuff

last thing is explain algorithm
