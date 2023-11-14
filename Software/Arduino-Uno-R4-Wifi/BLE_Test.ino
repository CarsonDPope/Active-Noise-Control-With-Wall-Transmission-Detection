/******************************************************************************************************
 
  File Name:   BLE_Test.ino
  Authors:     Carson Pope, Jared Vega, Caleb Turney
  Date:        October 31, 2023
  Description: Connects to another device via BLE to transmit
               one byte of information back and forth. Specifically, 
               it connects and waits for the other device to start by
               updating its value then replies with a byte to send. 
               Both devices should wait to reply until the other responds,
               creating a feedback loop.

******************************************************************************************************/

#include <ArduinoBLE.h>

const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* sendCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";
const char* receiveCharacteristicUuid = "19b10002-e8f2-537e-4f6c-d104768a1214";

byte send = -2;
byte received = 0;


void setup() {

  Serial.begin(9600);

  while (!Serial)
    ;

  if (!BLE.begin()) {
    Serial.println("* Starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  BLE.setLocalName("Arduino Uno R4");
  BLE.advertise();
  Serial.println("Arduino Uno R4 (Central Device)");
  Serial.println(" ");
}  // setup


void loop() {
  connectToPeripheral();
}  // loop


void connectToPeripheral() {

  BLEDevice peripheral;
  Serial.println("- Discovering peripheral device...");

  do {
    BLE.scanForUuid(deviceServiceUuid);
    peripheral = BLE.available();
  } while (!peripheral);

  if (peripheral) {
    Serial.println("* Peripheral device found!");
    Serial.print("* Device name: ");
    Serial.println(peripheral.localName());

    
    controlPeripheral(peripheral);
    /*
    Serial.print("* Device MAC address: ");
    Serial.println(peripheral.address());
    Serial.print("* Advertised service UUID: ");
    Serial.println(peripheral.advertisedServiceUuid());
    Serial.println(" ");
    */

    BLE.stopScan();
  }
}  // connectToPeripheral


void controlPeripheral(BLEDevice peripheral) {

  Serial.println("- Connecting to peripheral device...");

  if (peripheral.connect()) {
    Serial.println("* Connected to peripheral device!");
    Serial.println(" ");
  } else {
    Serial.println("* Connection to peripheral device failed!");
    Serial.println(" ");
    return;
  }

  Serial.println("- Discovering peripheral device attributes...");

  if (peripheral.discoverAttributes()) {
    Serial.println("* Peripheral device attributes discovered!");
    Serial.println(" ");
  } else {
    Serial.println("* Peripheral device attributes discovery failed!");
    Serial.println(" ");
    peripheral.disconnect();
    return;
  }


  BLECharacteristic sendCharacteristic = peripheral.characteristic(sendCharacteristicUuid);
  BLECharacteristic receiveCharacteristic = peripheral.characteristic(receiveCharacteristicUuid);


  if (!sendCharacteristic) {
    Serial.println("* Peripheral device does not have gesture_type characteristic!");
    peripheral.disconnect();
    return;
  } else if (!sendCharacteristic.canWrite()) {
    Serial.println("* Peripheral does not have a writable gesture_type characteristic!");
    peripheral.disconnect();
    return;
  }
  // subscribe to the simple key characteristic
  Serial.println("Subscribing to received characteristic ...");

  if (!receiveCharacteristic) {
    Serial.println("no receive characteristic found!");
    peripheral.disconnect();
    return;
  } else if (!receiveCharacteristic.canSubscribe()) {
    Serial.println("receive characteristic is not subscribable!");
    peripheral.disconnect();
    return;
  } else if (!receiveCharacteristic.subscribe()) {
    Serial.println("subscription failed!");
    peripheral.disconnect();
    return;
  }


  while (peripheral.connected()) {

    if (receiveCharacteristic.valueUpdated()) {
      receiveCharacteristic.readValue(received);
      Serial.print("** Reading value from other device...");
      Serial.println(received);

      send += 2;
      send = send % 100;

      Serial.print("* Writing value to send characteristic: ");
      Serial.println(send);
      sendCharacteristic.writeValue(send);
      //Serial.println("* Writing value to send characteristic done!");
      Serial.println(" ");
    }
    //Serial.println("- Peripheral device disconnected!");
  }
}  // controlPeripheral
