
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
 * Last Updated: 11/1/2023
 * ------------------------------------------------- 
 * =================================================
 * *************************************************/

#include <ArduinoBLE.h>

// UUID declarations
const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceGetCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";
const char* deviceSendCharacteristicUuid = "19b10002-e8f2-537e-4f6c-d104768a1214";

// Data variable declarations
byte get = -1;
byte send = -1;

// Service and Characteristic declarations
BLEService transactionService(deviceServiceUuid);
BLEByteCharacteristic getCharacteristic(deviceGetCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic sendCharacteristic(deviceSendCharacteristicUuid, BLERead | BLEIndicate);

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
          Serial.println(get);
          
          send += 1; // <--- this block for debugging
          send = send % 15 + 25;
          
          // sends value
          sendCharacteristic.writeValue((byte)send);
        
        } // end Wait to get
     
      } // end Connected Loop
    
    } // end Wait for subscription
    
    Serial.println("* Disconnected from central device!");
  
  } // end central connection
  
} // end Loop()