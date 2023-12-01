## Function:  
Stores audio samples streamed from blackfin through SPI in an array for comms, also writes the maximum value in the audio array to serial port of Arduino UNO R4 to integrate with the FxLMS algorithm
## HOW TO USE:
To use this code it is recommended to run it on the Python IDLE IDE. When ran this code will ask the user to select the port of interest which will be the corresponding USB COM port the Arduino 33 Nano IOT is currently connected to. You will also need a pre-trained model. Attached in this software folder is a pre-trained model called Capstone.keras this will allow the program to be ran. The program can also be modified to run any pre-trained models the user desires as long as they use the appropriate NAME.keras file and run thier code in the same file directory in which the NAME.keras file is located.
### Required Pyton Extensions
    In order to use this program you must also have the following python extensions installed:
    -numpy
    -Pyserial
    -tensorflow
    -sys
    -keras
    
