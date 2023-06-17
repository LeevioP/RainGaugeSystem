/*
  Rui Santos
  Complete project details at our blog: https://RandomNerdTutorials.com/esp32-esp8266-firebase-bme280-rtdb/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <DHT.h>

// #define BUILT_IN_LED 13 //Pin 13 -> LED
#define BUCKET 5 //Pin 5 -> Tipping Bucket
#define DHTPIN 13 //Pin 13 -> data DHT11
#define DHTTYPE DHT11 

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "LwpLaxy"
#define WIFI_PASSWORD "tpvq7672"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCghNrrDNxGwf9p4oFkU-pBfYTeNVtPsQ8"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "purnamawanglw20d@student.unhas.ac.id"
#define USER_PASSWORD "Celcius23"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://esp-rpl-default-rtdb.asia-southeast1.firebasedatabase.app"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;
String tempPath;
String humPath;
String rainPath;

//DHT11
DHT dht(DHTPIN, DHTTYPE);
float temperature;
float humidity;

//Tipping Bucket
float tippingCount;
int reads;
int state;
float gauge;
float rainfall;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 60000;

// Initialize BME280
// void initBME(){
//   if (!bme.begin(0x76)) {
//     Serial.println("Could not find a valid BME280 sensor, check wiring!");
//     while (1);
//   }
// }

void rainGauge(){
  reads = digitalRead(BUCKET);
  switch(state) {
    case 0: {
      if (!reads) {
        tippingCount++;
        // digitalWrite(BUILT_IN_LED,HIGH);
        state=1;    
      }
      break;
    }
    case 1: {
      if (reads) {
        // digitalWrite(BUILT_IN_LED,LOW);
        state=0;    
      }
      break;
    }
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Write float values to the database
void sendFloat(String path, float value){
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value)){
    Serial.print("Writing value: ");
    Serial.print (value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  }
  else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void setup(){
  Serial.begin(115200);

  //Tipping BUcket
  pinMode(BUCKET,INPUT_PULLUP);

  // Initialize BME280 sensor
  // initBME();
  initWiFi();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid;

  // Update database path for sensor readings
  tempPath = databasePath + "/temperature"; // --> UsersData/<user_uid>/temperature
  humPath = databasePath + "/humidity"; // --> UsersData/<user_uid>/humidity
  rainPath = databasePath + "/rainfall"; // --> UsersData/<user_uid>/rainfall
}

void loop(){
  rainGauge();

  // Send new readings to database
  if (Firebase.ready() && ((millis() - sendDataPrevMillis) > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    // Get latest sensor readings    
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    gauge = tippingCount*0.657;
    // if (isnan(humidity) || isnan(temperature)) { //jika tidak ada hasil
    // Serial.println("DHT11 tidak terbaca... !");
    // return;
    // }

    // Send readings to database:
    sendFloat(tempPath, temperature);
    sendFloat(humPath, humidity);
    sendFloat(rainPath, gauge);
    tippingCount = 0;
  }
}