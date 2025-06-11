#include <Wire.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Servo.h>

Servo servo1;     
Servo servo2;    


#define capPin D3             
#define trigPin D4            
#define echoPin D5           

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define API_KEY ""
#define DATABASE_URL ""

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

unsigned long sendDataPrevMillis = 0;
bool plasticHandled = false;

void setup() {
  Serial.begin(9600);
  
  pinMode(capPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  servo1.attach(D1);     
  servo2.attach(D2);   

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to Wi-Fi");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
    Serial.println("Firebase connected");
  } else {
    Serial.printf("Firebase error: %s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 500 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    int capValue = digitalRead(capPin); 
    long distance = getUltrasonic();

    Serial.print("Capacitive: ");
    Serial.print(capValue);
    Serial.print(" | Distance: ");
    Serial.println(distance);

    if (capValue == 0) {
      Serial.println("Solid detected");
      Firebase.RTDB.setInt(&fbdo, "Detection/isBottle", false);
      servo1.write(180);
      delay(2000);
      servo1.write(0);
      plasticHandled = false;  
    } else if (capValue == 1 && distance < 5 && !plasticHandled) {
      Serial.println("Plastic bottle detected");
      Firebase.RTDB.setInt(&fbdo, "Detection/isBottle", true);
      servo2.write(180);
      plasticHandled = true; 
      delay(2000);
      Firebase.RTDB.setInt(&fbdo, "Detection/isBottle", false);
    } else if (plasticHandled && distance < 5) {
      Firebase.RTDB.setInt(&fbdo, "Detection/isBottle", true);
      servo2.write(0);
      plasticHandled = false;
      delay(2000);
      Firebase.RTDB.setInt(&fbdo, "Detection/isBottle", false);
    }
    delay(100);
  }
}

long getUltrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long distanceCm = duration * 0.034 / 2;
  return distanceCm;

  
}
