/* *************************************************
 * Arduino SPI Test 
 * =================================================
 * Function: Sends and receives byte data to another
 *           device via SPI. Runs SPI based on a 
 *           timer or hardware interrupt. To test
 *           with itself, connect pins 11 and 12
 * -------------------------------------------------
 * Authors: Jared Vega
 * Last Updated: 11/08/2023
 * ------------------------------------------------- 
 * =================================================
 * *************************************************/
#include <SPI.h>


unsigned long lastMillis = 0;
unsigned long currentMillis = 0;
const int timerInterval = 1000; // Minimum time between SPI transactions


// Uno R4 Wifi can only use pins 2 or 3 for interrupts
const int interruptPin = 2;
const int readyPin = 3;

volatile bool bRunSPI = false;

byte send = 0;
byte received = 0;

void setup(){
  //Help with Blackfin's SPI
  pinMode(readyPin, OUTPUT);
  digitalWrite(readyPin, HIGH);

  Serial.begin(9600);


  // Set interrupt pin as input, can use 2 or 3
  pinMode(interruptPin, INPUT);

  // Allows SPI to be used within an interrupt without conflicts
  SPI.usingInterrupt(digitalPinToInterrupt(interruptPin));

  // When interrupt pin changes, call the function: void ISR_SPI(){ }
  attachInterrupt(digitalPinToInterrupt(interruptPin), ISR_SPI, CHANGE);

  // Turn on Interrupts
  SPI.attachInterrupt();

  /* SCK = Serial Clock,               Pin 13
  * MOSI = Master out Slave in,       Pin ~11
  * SS = Slave Select / Chip Select,  Pin ~10
  * MISO = Master in Slave out,       Pin 12
  * Initializes the SPI bus by setting SCK, MOSI, and SS to outputs, pulling SCK and MOSI low, and SS high.
  */
  SPI.begin();

  // Default speed set to 4MHz, SPI mode set to MODE 0 (polarity (CPOL) 0, phase (CPHA) 0) and Bit order set to MSB first.
  SPI.beginTransaction(SPISettings());

}



void loop(){

  unsigned long currentMillis = millis();

  // Only way for now to run an 'interrupt' is to poll, as the Uno R4 uses a new microcontroller
  // that does not support AVR architecture (i.e. touching specific registers) 
  // like every library out there currently does
  if (currentMillis - lastMillis >= timerInterval) {
    timer_ISR();
    lastMillis = currentMillis;
  }

  if(bRunSPI == true){
    send += 2;
    received = SPI.transfer(send+1);
    Serial.print("* Sending value to SPI: ");
    Serial.println(send);
    Serial.print("* Value received from SPI: ");
    Serial.println(received);
    Serial.print("****************************\n\n");
    bRunSPI = false;
  }
}// Loop()

// SPI interrupt function
// Runs when Pin 2 detects a change (high to low or low to high)
void ISR_SPI() {
  bRunSPI = true;  
}

// SPI timer function
// Runs every x milliseconds (or longer, as this works by polling) defined by timerInterval
void timer_ISR(){
  bRunSPI = true;
}

//Ignore below
/*
byte send = 2;
byte receive = 0;
void setup() {
  Serial.begin(9600);
  
  // Not sure if this is needed
  pinMode(10, OUTPUT); // set the SS pin as an output

  //Initializes the SPI bus by setting SCK, MOSI, and CS/SS to outputs, pulling SCK and MOSI low, and CS/SS high.
  SPI.begin();

  // Default speed set to 4MHz, SPI mode set to MODE 0: (clock polarity (CPOL) 0, phase (CPHA) 0) Output edge falling and Data capture rising, and Bit order set to MSB first.
  //                               400 MHz    
  SPI.beginTransaction(SPISettings(400000000, MSBFIRST, SPI_MODE0));

  digitalWrite(10, LOW);  // set the chip select as low to receive SPI (pin 10 is a "not" pin)
}


void loop() {
  delay(200);
  send += 2;
  send = send % 100;
  Serial.print(" Sending value to Shark: ");
  Serial.println(send);

  receive = SPI.transfer(send);

  Serial.print(" Received value: ");
  Serial.println(receive);
  Serial.println("***************************");
  Serial.println(" ");
}
*/
