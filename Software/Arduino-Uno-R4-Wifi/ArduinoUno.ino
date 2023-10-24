/*
  The goal of this program is to communicate back and forth between a SPI interface and a BLE interface,
  acting as the medium to transfer information.

            SPI                  BLE
  Sharkfin <===> Arduino Uno R4 <===> Other Arduino
*/

#include <ArduinoBLE.h>
#include <SPI.h>
#include <CircularBuffer.h>

// ~~~~~~~~~~~~ BLE Setup ~~~~~~~~~~~~ //

const int baud_rate = 9600;
const int characteristicSize = 512;  //Max size of ble communication
byte tempArray[20];
BLEService chatService("7DEF8317-7300-4EE6-8849-46FACE74CA2A");
// Define characteristics for receiving and transmitting messages
// BLERead - characteristics can be read by connected devices
// BLENotify - characteristic can send notifications
// BLEWrite - characteristic can be written to by connected devices

// remote clients will be able to get notifications if this characteristic changes
BLECharacteristic txCharacteristic("7DEF8317-7301-4EE6-8849-46FACE74CA2A", BLERead | BLENotify, 20);
// remote clients can change this value for this code to read
BLECharCharacteristic rxCharacteristic("7DEF8317-7302-4EE6-8849-46FACE74CA2A", BLEWrite);


// Uno R4 Wifi can only use 2 or 3
const int interruptPin = 2;

// Size 100 circular buffer initialization
// This takes values from the Sharkfin to the other Arduino
CircularBuffer<char, 100> bufferToArduino;
/* Functions include: push, pop, clear, isEmpty, isFull, first, last, size
  push() adds to the tail, unshift() adds to the head
  pop() removes and returns tail, shift() removes and returns head
*/

// ~~~~~~~~~~~~ SPI Setup ~~~~~~~~~~~~ //

//This takes values from the Arduino to the Sharkfin
CircularBuffer<char, 100> bufferToShark;
const int bufferSize = 100;

void setup() {
  // ~~~~~~~~~~~~ BLE Setup ~~~~~~~~~~~~ //

  //Configure serial communication at given baud rate
  Serial.begin(baud_rate);
  //while (!Serial);
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (true)
      ;
  }

  //Built-in LED to see if a device connected
  pinMode(LED_BUILTIN, OUTPUT);

  BLE.setLocalName("Capstone: Arduino Uno R4 Wifi");

  // Set the UUID for the service to advertise
  BLE.setAdvertisedService(chatService);

  // Add characteristics to the service
  chatService.addCharacteristic(rxCharacteristic);
  chatService.addCharacteristic(txCharacteristic);

  // Assign event handlers for connected and disconnected
  BLE.setEventHandler(BLEConnected, connectHandler);
  BLE.setEventHandler(BLEDisconnected, disconnectHandler);

  // Assign event handler for rx characteristic
  rxCharacteristic.setEventHandler(BLEWritten, incomingDataHandler);

  // Add the service and set a value for the characteristic:
  BLE.addService(chatService);
  // Start advertising
  BLE.advertise();


  // ~~~~~~~~~~~~ SPI Setup ~~~~~~~~~~~~ //

  // Set interrupt pin as input
  pinMode(interruptPin, INPUT);

  //Initializes the SPI bus by setting SCK, MOSI, and SS to outputs, pulling SCK and MOSI low, and SS high.
  SPI.begin();

  // Default speed set to 4MHz, SPI mode set to MODE 0 (polarity (CPOL) 0, phase (CPHA) 0) and Bit order set to MSB first.
  SPI.beginTransaction(SPISettings());

  // Allows SPI to be used within an interrupt without conflicts
  SPI.usingInterrupt(digitalPinToInterrupt(interruptPin));

  // When interrupt pin goes from high to low, call the function: void ISR_SPI(){ }
  attachInterrupt(digitalPinToInterrupt(interruptPin), ISR_SPI, FALLING);

  // Turn on Interrupts
  SPI.attachInterrupt();
}


// SPI interrupt function, cannot take parameters or return values
// ACK Commands: 1 = Ack, 2 = no space in buffer(s), 3 =
void ISR_SPI() {
  // Data is transfering to and from Sharkfin to this board
  // If there are at least 20 open spots in the buffer and 20 elements to give to shark, SPI transfer 20 times
  if (bufferToArduino.available() > 20 && bufferToShark.size() > 20) {
    SPI.transfer(1));
    for (int i = 0; i < 20; i++) {
      bufferToArduino.push(SPI.transfer(bufferToShark.shift()));
    }
  } else {
    SPI.transfer(2);
  }
}

//Main Function
void loop() {

  // Calls event handlers when they get triggered
  BLE.poll();

  // Test values to send to other Arduino via BLE until more specifics can be made
  byte temp[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
  txCharacteristic.setValue(temp, 20);
  delay(1000);
}

// Runs when rxCharacteristic is updated I think
void incomingDataHandler(BLEDevice central, BLECharacteristic characteristic) {

  //Read the updated value, 20 bytes in length
  rxCharacteristic.readValue(tempArray, 20);
  for (int i = 0; i < 20; i++) {
    bufferToShark.push(tempArray[i]);
  }
  //Write sent array to serial for testing
  Serial.write(tempArray, 20);

  //When my other characteristic is updated, send my char to other Arduino
  if (!bufferToArduino.isEmpty()) {
    for (int i = 0; i < 20; i++) {
      tempArray[i] = bufferToArduino.shift();
    }
    // Send a 20 length array of bytes back to other Arduino
    txCharacteristic.setValue(tempArray, 20);
  }

  //Write received array to serial for testing
  Serial.write(tempArray, 20);
}

// Runs when a device connects via BLE
void connectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  digitalWrite(LED_BUILTIN, HIGH);
}

// Runs when a device disconnects
void disconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  digitalWrite(LED_BUILTIN, LOW);
}
