#include <ArduinoBLE.h>
#include <Wire.h>
#include <SparkFun_TMP117.h>
#define byteSize 8
BLEService batteryService("1822");

BLECharacteristic heartLevelChar("2A92",  // standard 16-bit characteristic UUID
                                 BLERead | BLENotify, byteSize); // remote clients will be able to get notifications if this characteristic changes
BLECharacteristic oxyLevelChar("2A62",  // standard 16-bit characteristic UUID
                               BLERead | BLENotify, byteSize); // remote clients will be able to get notifications if this characteristic changes
BLECharacteristic tempLevelChar("2A1C",  // standard 16-bit characteristic UUID
                                BLERead | BLENotify, byteSize); // remote clients will be able to get notifications if this characteristic changes

BLECharacteristic respLevelChar("2A90",  // standard 16-bit characteristic UUID
                                BLERead | BLENotify, 10); // remote clients will be able to get notifications if this characteristic changes
BLECharCharacteristic switchLevelChar("1111",  // standard 16-bit characteristic UUID
                                      BLEWrite); // remote clients will be able to get notifications if this characteristic changes

boolean dat = '0';

byte buffer[byteSize]; byte buffer1[byteSize]; byte buffer2[byteSize];
byte buf[10];
String string;
TMP117 sensor; // Initalize sensor

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Wire.setClock(400000);   // Set clock speed to be the fastest for better communication (fast mode)

  Serial.println("TMP117 Example 1: Basic Readings");
  if (sensor.begin() == true) // Function to check if the sensor will correctly self-identify with the proper Device ID/Address
  {
    Serial.println("Begin");
  }
  else
  {
    Serial.println("Device failed to setup- Freezing code.");
    while (1); // Runs forever
  }

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setLocalName("HOPS_BABY_MONITORING");
  BLE.setAdvertisedService(batteryService); // add the service UUID
  batteryService.addCharacteristic(heartLevelChar); // add the battery level characteristic
  batteryService.addCharacteristic(oxyLevelChar); // add the battery level characteristic
  batteryService.addCharacteristic(respLevelChar); // add the battery level characteristic
  batteryService.addCharacteristic(switchLevelChar); // add the battery level characteristic
  batteryService.addCharacteristic(tempLevelChar); // add the battery level characteristic

  BLE.addService(batteryService); // Add the battery service
  BLE.advertise();
  Serial.println("BLE Central - Peripheral Explorer");
  BLE.scan();
}

void loop() {
  BLEDevice central = BLE.central();
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    if (peripheral.localName() == "SPo2_Resp_BLE_HOPS") {
      BLE.stopScan();
      explorerPeripheral(peripheral);
      while (1) {
        explorerPeripheral(peripheral);
      }
    }
  }
}

void explorerPeripheral(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");

  if (peripheral.connect()) {
    Serial.println("Connected");
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    if (peripheral) {
      if (peripheral.localName() == "SPo2_Resp_BLE_HOPS") {
        BLE.stopScan();
      }
    }
    Serial.println("Failed to connect!");
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // read and print device name of peripheral
  Serial.println();
  Serial.print("Device name: ");
  Serial.println(peripheral.deviceName());
  Serial.print("Appearance: 0x");
  Serial.println(peripheral.appearance(), HEX);
  Serial.println();

  // loop the services of the peripheral and explore each
  for (int i = 0; i < peripheral.serviceCount(); i++) {
    BLEService service = peripheral.service(i);

    exploreService(service);
  }

  BLECharacteristic heartCharacteristic = peripheral.characteristic("2A62");
  BLECharacteristic oxyCharacteristic = peripheral.characteristic("2A92");
  BLECharacteristic respCharacteristic = peripheral.characteristic("2A90");
  BLECharacteristic switchCharacteristic = peripheral.characteristic("0001");


  if (!heartCharacteristic) {
    Serial.println("heart Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  }
  else if (!heartCharacteristic.canSubscribe()) {
    Serial.println("heart characteristic is not subscribable!");
    peripheral.disconnect();
    dat = '0';
    return;
  } else if (!heartCharacteristic.subscribe()) {
    Serial.println("heart subscription failed!");
    peripheral.disconnect();
    return;
  } else {
    Serial.println("heart Subscribed");
    dat = '1';
  }
  if (!oxyCharacteristic) {
    Serial.println("oxy Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  }
  else if (!oxyCharacteristic.canSubscribe()) {
    Serial.println("oxy characteristic is not subscribable!");
    peripheral.disconnect();
    dat = '0';
    return;
  } else if (!oxyCharacteristic.subscribe()) {
    Serial.println("oxy subscription failed!");
    peripheral.disconnect();
    return;
  } else {
    Serial.println(" oxy Subscribed");
    dat = '1';
  }

  if (!respCharacteristic) {
    Serial.println("resp Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  }
  else if (!respCharacteristic.canSubscribe()) {
    Serial.println("resp  characteristic is not subscribable!");
    peripheral.disconnect();
    dat = '0';
    return;
  } else if (!respCharacteristic.subscribe()) {
    Serial.println("resp  subscription failed!");
    peripheral.disconnect();
    return;
  } else {
    Serial.println(" resp Subscribed");
    dat = '1';
  }

  if (!switchCharacteristic) {
    Serial.println("switch Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  }
  else if (!switchCharacteristic.canWrite()) {
    Serial.println(" switch Peripheral does not have a writable LED characteristic!");
    peripheral.disconnect();
    return;
  } else {
    Serial.println("switch Subscribed");
    dat = '1';
  }

  while (peripheral.connected()) {
    switchCharacteristic.writeValue((byte)0x03);
    delay(100);
    //    if (central) {
    //      while (central.connected()) {
    Serial.println(heartCharacteristic.readValue(buffer1, 0x0A));
    heartLevelChar.writeValue(buffer1, byteSize);
    printData(buffer1, sizeof(buffer1));

    Serial.println(oxyCharacteristic.readValue(buffer2, 0x0A));
    oxyLevelChar.writeValue(buffer2, byteSize);
    printData(buffer2, sizeof(buffer2));

    Serial.println(respCharacteristic.readValue(buffer, 0x0A));
    respLevelChar.writeValue(buffer, byteSize);
    printData(buffer, sizeof(buffer));

    if (sensor.dataReady() == true) // Function to make sure that there is data ready to be printed, only prints temperature values when data is ready
    {
      float tempF = sensor.readTempF();
      string = String(tempF);
      Serial.println(string);
      string.getBytes(buffer, 10);
      Serial.print("Temperature in Fahrenheit: ");
      tempLevelChar.writeValue(buffer, byteSize);
    }
  }
}

void exploreService(BLEService service) {
  // print the UUID of the service
  Serial.print("Service ");
  Serial.println(service.uuid());

  // loop the characteristics of the service and explore each
  for (int i = 0; i < service.characteristicCount(); i++) {
    BLECharacteristic characteristic = service.characteristic(i);

    exploreCharacteristic(characteristic);
  }
}

void exploreCharacteristic(BLECharacteristic characteristic) {
  // print the UUID and properties of the characteristic
  Serial.print("\tCharacteristic ");
  Serial.print(characteristic.uuid());
  Serial.print(", properties 0x");
  Serial.print(characteristic.properties(), HEX);

  // check if the characteristic is readable
  if (characteristic.canRead()) {
    // read the characteristic value
    characteristic.read();

    if (characteristic.valueLength() > 0) {
      // print out the value of the characteristic
      Serial.print(", value 0x");
      printData(characteristic.value(), characteristic.valueLength());
    }
  }
  Serial.println();

  // loop the descriptors of the characteristic and explore each
  for (int i = 0; i < characteristic.descriptorCount(); i++) {
    BLEDescriptor descriptor = characteristic.descriptor(i);

    exploreDescriptor(descriptor);
  }
}

void exploreDescriptor(BLEDescriptor descriptor) {
  // print the UUID of the descriptor
  Serial.print("\t\tDescriptor ");
  Serial.print(descriptor.uuid());

  // read the descriptor value
  descriptor.read();

  // print out the value of the descriptor
  Serial.print(", value 0x");
  printData(descriptor.value(), descriptor.valueLength());

  Serial.println();
}

void printData(const unsigned char data[], int length) {
  for (int i = 0; i < length; i++) {
    unsigned char b = data[i];

    if (b < 16) {
      Serial.print("0");
    }

    buf[i] = data[i];

    Serial.print(char(b));
    //        Serial.println();
  }

  Serial.println();
}
