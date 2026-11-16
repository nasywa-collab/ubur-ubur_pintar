#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DHT.h>

// ================= KONFIGURASI WI-FI & MQTT =================
const char* ssid        = "Wokwi-GUEST";
const char* password    = "";
const char* mqtt_server = "broker.hivemq.com";

// Topic MQTT
const char* topic_log     = "smartlab/galih1010/log";
const char* topic_control = "smartlab/galih1010/control";
const char* topic_env     = "smartlab/galih1010/suhu";

WiFiClient espClient;
PubSubClient client(espClient);

// ================= KONFIGURASI PIN HARDWARE =================
#define SS_PIN    5
#define RST_PIN   4
#define SERVO_PIN 13
#define LED_GREEN 12
#define LED_RED   14
#define BUZZER    27

#define EXIT_BTN  26
#define DHT_PIN   25
#define RELAY_PIN 33
#define DHT_TYPE  DHT22

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo doorServo;
DHT dht(DHT_PIN, DHT_TYPE);

struct CardUser {
  byte uid[4];
  String name;
};

CardUser knownUsers[] = {
  {{0x04, 0x11, 0x22, 0x33}, "User 1"},  // Blue Card
  {{0xC0, 0xFF, 0xEE, 0x99}, "User 2"}   // Red Card
};

const int totalKnownUsers = sizeof(knownUsers) / sizeof(knownUsers[0]);
unsigned long lastEnvSend = 0;

// Deklarasi Fungsi (Wajib untuk C++ .cpp)
void updateEnvironment();
void bukaPintu(String judul);
void tolakAkses();
String getUserName(byte *cardUID, byte cardSize);
void showStandbyMessage();

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Pesan MQTT masuk [" + String(topic) + "]: " + message);

  if (String(topic) == topic_control) {
    if (message == "OPEN") {
      client.publish(topic_log, "Pintu dibuka jarak jauh oleh Admin!");
      digitalWrite(RELAY_PIN, HIGH);
      bukaPintu("Remote Admin");
      showStandbyMessage();
    } 
    else if (message == "LAMP_ON") {
      digitalWrite(RELAY_PIN, HIGH);
      client.publish(topic_log, "Lampu Lab: DINYALAKAN oleh Admin");
      Serial.println("-> Relay Lampu ON");
    } 
    else if (message == "LAMP_OFF") {
      digitalWrite(RELAY_PIN, LOW);
      client.publish(topic_log, "Lampu Lab: DIMATIKAN oleh Admin");
      Serial.println("-> Relay Lampu OFF");
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT Broker...");
    String clientId = "ESP32-SmartLab-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Terhubung!");
      client.subscribe(topic_control);
      client.publish(topic_log, "System Smart Lab All-in-One Online!");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi 2 detik...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, 5);
  rfid.PCD_Init();

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  dht.begin();

  ESP32PWM::allocateTimer(0);
  doorServo.setPeriodHertz(50);
  doorServo.attach(SERVO_PIN, 500, 2400);
  doorServo.write(0);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(EXIT_BTN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, LOW);

  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  showStandbyMessage();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (digitalRead(EXIT_BTN) == LOW) {
    Serial.println("-> Tombol Keluar Ditekan!");
    client.publish(topic_log, "EXIT: Seseorang keluar dari dalam Lab (Lampu Lab OFF)");
    digitalWrite(RELAY_PIN, LOW);
    bukaPintu("Silakan Keluar");
    showStandbyMessage();
  }

  if (millis() - lastEnvSend > 5000) {
    lastEnvSend = millis();
    updateEnvironment();
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  String userName = getUserName(rfid.uid.uidByte, rfid.uid.size);

  if (userName != "Stranger") {
    String logMsg = "AKSES DITERIMA: " + userName + " (Lampu Lab ON)";
    Serial.println("-> " + logMsg);
    client.publish(topic_log, logMsg.c_str());
    digitalWrite(RELAY_PIN, HIGH);
    bukaPintu(userName);
  } else {
    String alertMsg = "AKSES DITOLAK: Stranger mencoba masuk!";
    Serial.println("-> " + alertMsg);
    client.publish(topic_log, alertMsg.c_str());
    tolakAkses();
  }

  rfid.PICC_HaltA();
  showStandbyMessage();
}

void updateEnvironment() {
  float suhu = dht.readTemperature();
  float lembab = dht.readHumidity();

  if (!isnan(suhu) && !isnan(lembab)) {
    lcd.setCursor(0, 1);
    lcd.print("T:" + String(suhu, 1) + "C H:" + String(lembab, 0) + "%   ");

    String envPayload = "{\"suhu\":" + String(suhu, 1) + ",\"kelembapan\":" + String(lembab, 1) + "}";
    client.publish(topic_env, envPayload.c_str());
  }
}

void bukaPintu(String judul) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(judul);
  lcd.setCursor(0, 1);
  lcd.print(" Pintu Terbuka  ");

  digitalWrite(LED_GREEN, HIGH);

  tone(BUZZER, 1200); delay(100);
  noTone(BUZZER);     delay(80);
  tone(BUZZER, 1600); delay(120);
  noTone(BUZZER);

  doorServo.write(90);
  delay(3000);
  doorServo.write(0);
  digitalWrite(LED_GREEN, LOW);
}

void tolakAkses() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Halo, Stranger!");
  lcd.setCursor(0, 1);
  lcd.print(" Akses Ditolak! ");

  digitalWrite(LED_RED, HIGH);

  for (int i = 0; i < 3; i++) {
    tone(BUZZER, 500); delay(150);
    noTone(BUZZER);    delay(100);
  }

  delay(1500);
  digitalWrite(LED_RED, LOW);
}

String getUserName(byte *cardUID, byte cardSize) {
  for (int i = 0; i < totalKnownUsers; i++) {
    bool match = true;
    for (int j = 0; j < cardSize; j++) {
      if (cardUID[j] != knownUsers[i].uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return knownUsers[i].name;
  }
  return "Stranger";
}

void showStandbyMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" SMART LAB ACCESS ");
  updateEnvironment();
}