// ============================================================
//  hcsr04.h — Sensor Ultrasonik HC-SR04
//  Mendukung deteksi koneksi/diskoneksi sensor secara realtime
// ============================================================
#pragma once
#include <Arduino.h>
#include "config.h"

// ---- Konstanta tambahan untuk deteksi koneksi ----
#define HCSR04_CONN_TIMEOUT_US  30000UL   // 30 ms -> maks ~500 cm
#define HCSR04_CONN_MIN_CM      2
#define HCSR04_CONN_MAX_CM      400
#define HCSR04_CONN_ERR_THRESH  5          // Jumlah error berturut-turut untuk dianggap terputus
#define HCSR04_CONN_CHECK_MS    1000       // Cek koneksi setiap 1 detik

// ================================================================
//  INISIALISASI PIN SENSOR
// ================================================================
void hcsr04_init() {
    // Gerbang Masuk
    pinMode(PIN_TRIG1, OUTPUT);
    pinMode(PIN_ECHO1, INPUT);
    digitalWrite(PIN_TRIG1, LOW);
    // Gerbang Keluar
    pinMode(PIN_TRIG2, OUTPUT);
    pinMode(PIN_ECHO2, INPUT);
    digitalWrite(PIN_TRIG2, LOW);
}

// ================================================================
//  CEK KONEKSI SENSOR — true jika echo valid diterima
//  Fungsi ini mendeteksi apakah sensor terhubung (ada respons echo)
//  Tidak memeriksa jarak karena itu tergantung objek di depan
// ================================================================
bool hcsr04_is_connected(uint8_t trigPin, uint8_t echoPin) {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // Reset pin TRIG ke LOW dulu
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    // Kirim pulse 10us
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Baca durasi echo - jika > 0 berarti ada respons = sensor terhubung
    // Timeout 25ms = maks ~430cm, jika 0 = tidak ada respons = sensor terputus
    unsigned long duration = pulseIn(echoPin, HIGH, 25000);
    return (duration > 0);  // Sensor terhubung jika ada echo response
}

// ================================================================
//  CEK KONEKSI HC-SR04 MASUK (fungsi shortcut)
// ================================================================
bool hcsr04_masuk_connected() {
    return hcsr04_is_connected(PIN_TRIG1, PIN_ECHO1);
}

// ================================================================
//  CEK KONEKSI HC-SR04 KELUAR (fungsi shortcut)
// ================================================================
bool hcsr04_keluar_connected() {
    return hcsr04_is_connected(PIN_TRIG2, PIN_ECHO2);
}

/**
 * Ukur jarak dalam cm.
 * @param trigPin  Pin TRIG sensor
 * @param echoPin  Pin ECHO sensor
 * @return Jarak dalam cm, atau -1 jika timeout
 */
float hcsr04_read_cm(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
    if (duration == 0) return -1;

    float distance = duration * 0.034f / 2.0f;
    return distance;
}

/**
 * Apakah ada kendaraan di depan sensor?
 * @param trigPin  Pin TRIG sensor
 * @param echoPin  Pin ECHO sensor
 * @return true jika jarak < DETECT_DISTANCE_CM
 */
bool hcsr04_detected(int trigPin, int echoPin) {
    float dist = hcsr04_read_cm(trigPin, echoPin);
    if (dist < 0) return false; // timeout = tidak terdeteksi
    return (dist < DETECT_DISTANCE_CM);
}

// Shortcut: deteksi gerbang masuk
bool hcsr04_masuk_detected() {
    return hcsr04_detected(PIN_TRIG1, PIN_ECHO1);
}

// Shortcut: deteksi gerbang keluar
bool hcsr04_keluar_detected() {
    return hcsr04_detected(PIN_TRIG2, PIN_ECHO2);
}
