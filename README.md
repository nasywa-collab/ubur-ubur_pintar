# 🔐 Smart Lab Access & Automation System (ESP32 + IoT MQTT)

Sistem kontrol akses pintu laboratorium dan otomatisasi ruangan berbasis **ESP32** yang disimulasikan di **Wokwi** dan terintegrasi dengan **MQTT (`broker.hivemq.com`)**.

## ✨ Fitur Utama
- **Akses Pintu RFID:** Membuka pintu otomatis (Servo) untuk kartu terdaftar (`User 1`, `User 2`) dan membunyikan alarm buzzer untuk `Stranger`.
- **Smart Lighting (Relay):** Lampu lab menyala otomatis saat user masuk dan mati saat tombol keluar ditekan.
- **Monitoring Suhu (DHT22):** Menampilkan suhu & kelembapan ruangan di LCD serta mengirim data ke MQTT setiap 5 detik.
- **Remote Control Admin:** Kontrol buka pintu (`OPEN`) dan sakelar lampu (`LAMP_ON` / `LAMP_OFF`) jarak jauh via MQTT.

## 🔌 Pinout Utama ESP32
- **RFID RC522:** SDA `D5`, SCK `D18`, MOSI `D23`, MISO `D19`, RST `D4`
- **LCD 16x2 I2C:** SDA `D21`, SCL `D22`
- **Aktuator & Sensor:** Servo `D13`, DHT22 `D25`, Exit Button `D26`, Relay `D33`
- **Indikator:** LED Hijau `D12`, LED Merah `D14`, Buzzer `D27`

## 📡 Topik MQTT
- `smartlab/galih1010/log` — Log aktivitas pintu & status lampu (*Subscribe*)
- `smartlab/galih1010/suhu` — Data sensor suhu & kelembapan (*Subscribe*)
- `smartlab/galih1010/control` — Perintah admin: `OPEN`, `LAMP_ON`, `LAMP_OFF` (*Publish*)

## 🚀 Link Simulasi
- **Wokwi Project:** [Buka Simulasi Wokwi](https://wokwi.com/projects/480407276313118721)