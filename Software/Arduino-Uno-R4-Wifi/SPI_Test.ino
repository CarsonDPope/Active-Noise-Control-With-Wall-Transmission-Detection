#include <SPI.h>

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
