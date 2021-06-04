#include <SparkFun_Bio_Sensor_Hub_Library.h>
#include <ArduinoBLE.h>
#include <Wire.h>
#include <api/deprecated-avr-comp/avr/dtostrf.h>
#define byteSize 8

BLEService sp02Service("1822");
BLEService respService("1890");
BLEService startService("1891");


BLECharacteristic sp02Rate("2A62", BLERead | BLENotify, byteSize);
BLECharacteristic heartRate("2A92", BLERead | BLENotify, byteSize);
BLECharacteristic respRate("2A90", BLERead | BLENotify, byteSize);
BLEByteCharacteristic startRate("0001", BLENotify | BLEWrite);

String string, string1, string2;
byte buffer[byteSize]; byte buffer1[byteSize]; byte buffer2[byteSize];
int resPin = 4;
int mfioPin = 5;
SparkFun_Bio_Sensor_Hub bioHub(resPin, mfioPin);
bioData body;
volatile boolean start_flag = '0';
float temperature; float temperature1; float PR, oxy, conf;
long previousMillis = 0;
int button = 12; char buttonStatus = 1;
const int resp_pin = A0; const int data_len = 2000;
double _resp; double resp_arr[data_len]; double resp_sig; double beat[data_len]; unsigned char peak = 0;
uint32_t a, b; volatile double auto_thre = 100;
unsigned long startMillis;  unsigned long currentMillis; const unsigned long period = 10000;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(button, INPUT);
  digitalWrite(LED_BUILTIN, LOW);
  bootPulse();
  delay(4000);
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }
  BLE.setLocalName("SPo2_Resp_BLE_HOPS");
  sp02Service.addCharacteristic(sp02Rate); // add the battery level characteristic
  sp02Service.addCharacteristic(heartRate); // add the battery level characteristic
  sp02Service.addCharacteristic(respRate); // add the battery level characteristic
  sp02Service.addCharacteristic(startRate); // add the battery level characteristic

  BLE.addService(sp02Service); // Add the battery service
  //BLE.addService(respService); // Add the battery service
  //BLE.addService(startService); // Add the battery service
  //startRate.writeValue(0);

  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");

  startMillis = millis();  //initial start time
}

void loop() {
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  BLEDevice central = BLE.central();
  if (central) {
    startRate.writeValue(0);
    while (central.connected()) {
      digitalWrite(LED_BUILTIN, HIGH);
      if (startRate.value() == 0x03) {   // any value other than 0
        //Serial.println("Congratulations, Device is ready to use");
//        start_flag = '1';
        respi_plot();
      }
      else if (startRate.written()) {
        if (startRate.value() == '2') {
          resp_scan();
        }
      }
      else {                              // a 0 value
        Serial.println(F("LED off"));
      }
    }
  }
  start_flag = '0';
  digitalWrite(LED_BUILTIN, LOW);
}

float spo2() {
  body = bioHub.readBpm();
  Serial.print("Oxygen: ");
  Serial.println(oxy);
  Serial.print("heartrate: ");
  Serial.println(PR);
  Serial.print("Confidence: ");
  Serial.println(conf);
  return PR, oxy, conf;
}

void bootPulse() {
  Wire.begin();
  int result = bioHub.begin();
  if (result == 0) // Zero errors!
    Serial.println("Sensor started!");
  else
    Serial.println("Could not communicate with the sensor!!!");

  Serial.println("Configuring Sensor....");
  int error = bioHub.configBpm(MODE_ONE); // Configuring just the BPM settings.
  if (error == 0) { // Zero errors!
    Serial.println("Sensor configured.");
  }
  else {
    Serial.println("Error configuring sensor.");
    Serial.print("Error: ");
    Serial.println(error);
  }
  //  Serial.println("Loading up the buffer with data....");
}

void respi_mean() {
  uint32_t i;
  resp_sig = 0;
  for (i = 0; i < data_len; i++) {
    resp_arr[i] = analogRead(resp_pin);
    if (resp_arr[i] < 0) {
      resp_arr[i] = 0 ;
    }
    resp_sig = resp_sig + resp_arr[i];
    delay(10);
  }
  resp_sig = resp_sig / data_len;
  auto_thre = resp_sig;
  Serial.println(resp_sig);
  delay(100);
}

void respi_calc() {
  uint32_t i;
  resp_sig = 0;
  for (i = 0; i < data_len; i++) {
    resp_arr[i] = analogRead(resp_pin);
    if (resp_arr[i] < 0) {
      resp_arr[i] = 0 ;
    }
    resp_sig = resp_sig + resp_arr[i];
    delay(10);
  }
  resp_sig = resp_sig / data_len;
  resp_sig = resp_sig / 2;
  auto_thre = resp_sig;
  Serial.println(resp_sig);
  delay(100);
}

double respi_plot() {
  uint32_t i;
  boolean beat_r;
  resp_sig = 0;
  for (i = 0; i < data_len; i++) {
    resp_arr[i] = analogRead(resp_pin);
    if (resp_arr[i] < 0) {
      resp_arr[i] = 0 ;
    }
    resp_sig = resp_sig + resp_arr[i];
  }
  resp_sig = resp_sig / data_len;
  resp_sig = resp_sig - auto_thre;
  //  auto_thre = resp_sig;

  if (resp_sig > 100) {
    beat_r = 1;
    //    Serial.println(beat);
  }
  else {
    beat_r = 0;
    //    Serial.println(beat);
  }
  spo2();
  PR = body.heartRate;
  oxy = body.oxygen;
  conf = body.confidence;
  Serial.println(resp_sig);
  string = String(resp_sig);
  string.getBytes(buffer, byteSize);
  respRate.writeValue(buffer, byteSize);
  if (conf > 90) {
    string1 = String(PR);
    string1.getBytes(buffer1, byteSize);
    Serial.print(string1);
    Serial.println(PR);
    heartRate.writeValue(buffer1, byteSize);
    string2 = String(oxy);
    string2.getBytes(buffer2, byteSize);
    sp02Rate.writeValue(buffer2, byteSize);
    Serial.print("Oxygen: ");
    Serial.println(string2);
  }
  return beat_r;
}

double calc_count() {

}

void clear_buf() {
  for (int i = 0; i < sizeof(buffer); i++) {
    buffer[i] = 0;
  }
}

void resp_scan() {
  delay(1000);
  Serial.println("Scan Begins");
  delay(1000);
  respi_calc();
  Serial.println("scan Complete");
  delay(1000);
}
