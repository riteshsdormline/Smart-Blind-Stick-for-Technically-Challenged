#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#define IR_PIN 2
#define TRIG1 3
#define ECHO1 4
#define TRIG2 5
#define ECHO2 6
#define BUTTON 7
#define BUZZER 8

SoftwareSerial gpsSerial(9, 10);
SoftwareSerial sim800(11, 12);
TinyGPSPlus gps;

unsigned long lastGPSupdate = 0;

String latitude = "0";
String longitude = "0";

void setup() {
  pinMode(IR_PIN, INPUT);
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  Serial.begin(9600);
  gpsSerial.begin(9600);
  sim800.begin(9600);

  delay(2000);
  initSIM800();
}

void loop() {
  readGPS();
  obstacleCheck();
  checkButton();
  readSMS();
}

/* ---------- ULTRASONIC FUNCTION ---------- */
long readUltrasonic(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  return pulseIn(echo, HIGH) * 0.034 / 2;
}

/* ---------- OBSTACLE DETECTION ---------- */
void obstacleCheck() {
  bool irObstacle = digitalRead(IR_PIN) == LOW;
  long d1 = readUltrasonic(TRIG1, ECHO1);
  long d2 = readUltrasonic(TRIG2, ECHO2);

  if (irObstacle || d1 <= 50 || d2 <= 50) {
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
  }
}

/* ---------- GPS UPDATE ---------- */
void readGPS() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (millis() - lastGPSupdate > 5000 && gps.location.isValid()) {
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);
    lastGPSupdate = millis();
  }
}

/* ---------- BUTTON SOS ---------- */
void checkButton() {
  if (digitalRead(BUTTON) == LOW) {
    sendSOS();
    delay(2000);
  }
}

/* ---------- SIM800 INITIAL ---------- */
void initSIM800() {
  sim800.println("AT");
  delay(1000);
  sim800.println("AT+CMGF=1");
  delay(1000);
  sim800.println("AT+CNMI=1,2,0,0,0");
}

/* ---------- SEND SOS ---------- */
void sendSOS() {
  sim800.println("AT+CMGS=\"+91XXXXXXXXXX\"");
  delay(1000);
  sim800.print("SOS ALERT!\nLocation:\n");
  sim800.print("https://maps.google.com/?q=");
  sim800.print(latitude);
  sim800.print(",");
  sim800.print(longitude);
  sim800.write(26);
}

/* ---------- READ SMS ---------- */
void readSMS() {
  if (sim800.available()) {
    String sms = sim800.readString();
    sms.toLowerCase();

    if (sms.indexOf("1") != -1) sendSensorStatus();
    else if (sms.indexOf("2") != -1) sendLocation();
    else if (sms.indexOf("3") != -1) sendSignal();
    else sendInvalid();
  }
}

/* ---------- RESPONSES ---------- */
void sendSensorStatus() {
  sim800.println("AT+CMGS=\"+91XXXXXXXXXX\"");
  delay(500);
  sim800.println("Sensor Status:");
  sim800.println("IR Sensor: OK");
  sim800.println("Ultrasonic 1 & 2: OK");
  sim800.println("GPS: OK");
  sim800.println("SIM800L: OK");
  sim800.write(26);
}

void sendLocation() {
  sim800.println("AT+CMGS=\"+91XXXXXXXXXX\"");
  delay(500);
  sim800.print("Location:\nhttps://maps.google.com/?q=");
  sim800.print(latitude);
  sim800.print(",");
  sim800.print(longitude);
  sim800.write(26);
}

void sendSignal() {
  sim800.println("AT+CSQ");
  delay(500);
  sim800.println("AT+CMGS=\"+91XXXXXXXXXX\"");
  delay(500);
  sim800.println("Carrier Signal Sent");
  sim800.write(26);
}

void sendInvalid() {
  sim800.println("AT+CMGS=\"+91XXXXXXXXXX\"");
  delay(500);
  sim800.println("Invalid Command. Tap again.\n1.Status\n2.Location\n3.Signal");
  sim800.write(26);
}
