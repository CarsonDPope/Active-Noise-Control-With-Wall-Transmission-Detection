/* *************************************************
 * Arduino Bluetooth Code (Laptop Side)
 * =================================================
 * Function: Receives audio bytes arduino uno r4
 *           through bluetooth low energy (BLE).
 *           Then sends that value to serial for 
 *           machine learning code to receive.
 *           Also receives (needs work) from machine
 *           learning code to send back to Uno R4.
 * -------------------------------------------------
 * Authors: Carson Pope, Caleb Turney, Jared Vega
 * Last Updated: 11/3/2023
 * ------------------------------------------------- 
 * =================================================
 * *************************************************/

#include <ArduinoBLE.h>
#include <CircularBuffer.h>

// UUID declarations
const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceGetCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";
const char* deviceSendCharacteristicUuid = "19b10002-e8f2-537e-4f6c-d104768a1214";
char from_laptop[1000];
CircularBuffer<byte,1000> data_send;

// Data variable declarations
byte get = -1;
byte send = -1;

// Service and Characteristic declarations
BLEService transactionService(deviceServiceUuid);
BLEByteCharacteristic getCharacteristic(deviceGetCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic sendCharacteristic(deviceSendCharacteristicUuid, BLERead | BLEIndicate);

// Size of the byte arrays
int arraySize = 100;

// Number of byte arrays
int numArrays = 3;

byte byteArr1 [100];
byte byteArr2 [100];
byte byteArr3 [100];
int maxi = 0;
int cycle = 0;

// Begin setup()
void setup() {
  
  Serial.begin(9600);
  while (!Serial);
  
  // Begin Bluetooth Module 
  if (!BLE.begin()) {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  // Setup Services and Characteristics to BLE module
  BLE.setLocalName("Arduino Nano 33 BLE (Peripheral)"); 
  BLE.setAdvertisedService(transactionService);         
  transactionService.addCharacteristic(getCharacteristic);
  transactionService.addCharacteristic(sendCharacteristic);
  BLE.addService(transactionService);
  getCharacteristic.writeValue(get);
  BLE.advertise();

  Serial.println("Nano 33 BLE (Peripheral Device)"); 
  Serial.println(" ");
  
  // Seed the random number generator
  randomSeed(analogRead(0));

  
  // Fill the byte array with random numbers between 0 and 200
  for (int i = 0; i < 100; i++) {
    byteArr1[i] = random(201); // Generates random numbers from 0 to 200
    byteArr2[i] = random(201); // Generates random numbers from 0 to 200
    byteArr3[i] = random(201); // Generates random numbers from 0 to 200
  }
  maxi = byteArr1[0];
    for (int i = 1; i < 100; i++) {
        if (byteArr1[i] > maxi) {
            maxi = byteArr1[i];
        }
    }
   // Serial.print("Highest of array 1: ");
   // Serial.println(maxi);

  maxi = byteArr2[0];
    for (int i = 1; i < 100; i++) {
        if (byteArr1[i] > maxi) {
            maxi = byteArr2[i];
        }
    }
   // Serial.print("Highest of array 2: ");
   // Serial.println(maxi);

  maxi = byteArr3[0];
    for (int i = 1; i < 100; i++) {
        if (byteArr3[i] > maxi) {
            maxi = byteArr1[i];
        }
    }
  //  Serial.print("Highest of array 3: ");
  //  Serial.println(maxi);
} // end Setup()

// Begin Loop()
void loop() {
  
  // Find Central
  BLEDevice central = BLE.central();
  Serial.println("- Discovering central device...");
  delay(500);
  
  if (central) {
    Serial.println("* Connected to central device!");
    Serial.print("* Device MAC address: ");
    Serial.println(central.address());
    Serial.println(" ");
    
    // Wait for central to subscribe
    if (sendCharacteristic.subscribed()) {
      
      // Start the transaction loop by sending a value
      Serial.print("Subscribed");
      sendCharacteristic.writeValue((byte)send);
      
      // Connected Loop
      while (central.connected()) {
      
        // Wait to get a value
        if (getCharacteristic.written()) {
        
          // gets value
          get = getCharacteristic.value();

          for(int i = 0; i<100; i++){
            switch(cycle % 3){
              case 0:
                get = byteArr1[i];
              break;
              case 1:
                get = byteArr2[i];
              break;
              case 2:
                get = byteArr3[i];  
              break;
            }
            if(Serial.availableForWrite() > 0){
              Serial.print(get);
            }
          }
            cycle++;

          //if(Serial.availableForWrite() > 0){
          //  Serial.print(get);
          // }


          if(Serial.available() > 0){
          send = Serial.read();
          }
          // sends value
          data_send.push(send);
          sendCharacteristic.writeValue(data_send.pop());
        
        } // end Wait to get
     
      } // end Connected Loop
    
    } // end Wait for subscription
    
    Serial.println("* Disconnected from central device!");
  
  } // end central connection
  
} // end Loop()

