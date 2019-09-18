#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include "tiger_config.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//  Sensor
int sensorState;

//  Deep Sleep
int deepSleepGpioState = 0;

const char* sensorLocalIP;

//  Delays
int generalDelay = 100;

void setup() {
  initialSetup();
}

void loop() {
  sensorState = getSensorState();
  sensorDidChangeState(sensorState);
  delay(generalDelay);
}

void initialSetup() {
  Serial.begin(115200);
  disableBrownoutDetector();
  initializeSensorGpio();
}

void disableBrownoutDetector(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
}

void initializeSensorGpio(){
  pinMode(interruptPin, INPUT_PULLUP);
}

boolean getSensorState() {
  return digitalRead(interruptPin);
}

void sensorDidChangeState(int state){
  Serial.println("Sensor Did Change State...");
  connectToWifiNetwork();

  if (state == 1) {
    gateDidOpen();
  } else {
    gateDidClose();
  }
 
  disconnectFromWifiNetwork();
  enableDeepSleep(deepSleepGpioState);
}

void gateDidOpen(){
  Serial.println("Gate Did Open");
  deepSleepGpioState = 0;
  sendMessage(1);
}

void gateDidClose(){
  Serial.println("Gate Did Close");
  deepSleepGpioState = 1;
  sendMessage(0);
}

void connectToWifiNetwork() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wifi... ");
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(generalDelay);
    Serial.print('.');
  }
  Serial.println("Connected!");
  delay(100);
  sensorLocalIP = WiFi.localIP().toString().c_str();
  delay(generalDelay);
}

void disconnectFromWifiNetwork(){
  Serial.println("Disconnect from Wifi...");
  WiFi.disconnect();
  delay(generalDelay);
}

void sendMessage(int sensorState) {
  HTTPClient http;
  String payload;
  http.begin(tigerHomeServerUrl);  //Specify destination for HTTP request
  http.addHeader("Content-Type", "application/json");
  const size_t capacity = JSON_OBJECT_SIZE(4);
  DynamicJsonDocument doc(capacity);

  doc["id"] = "ac0d2e82-b9fd-48e4-9162-4f38d089248d";
  doc["state"] = sensorState;
  doc["ipAddress"] = sensorLocalIP;
  
  serializeJson(doc, payload);
  http.POST(payload);   //Send the actual POST request
}

void enableDeepSleep(int stateAsInt) {
  Serial.println("Enable Deep Sleep...");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, stateAsInt);
  esp_deep_sleep_start();
}
