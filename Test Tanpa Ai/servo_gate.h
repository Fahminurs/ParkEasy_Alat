// ============================================================
//  servo_gate.h — Kontrol Servo Palang SG90
//  DETEKSI FISIK SERVO via ESP32 Touch Sensor (Hardware Kapasitansi)
// ============================================================
#ifndef SERVO_GATE_H
#define SERVO_GATE_H
#pragma once
#include <ESP32Servo.h>
#include "config.h"

// ---- Konfigurasi deteksi ----
#define SERVO_TOUCH_THRESHOLD  50   // Ubah jika false positive/negative
#define SERVO_TOUCH_SAMPLES    7    // Jumlah sampel per pengukuran
#define SERVO_DEBUG_TOUCH      0    // Set 1 untuk cetak nilai ke Serial

// ---- Status & posisi servo ----
inline bool servoMasukConnected  = false;
inline bool servoKeluarConnected = false;
inline int  servoMasukLastPos    = SERVO_CLOSE;
inline int  servoKeluarLastPos   = SERVO_CLOSE;

// Flag: palang sedang terbuka (jangan cek saat gate aktif)
inline bool servoMasukGateActive  = false;
inline bool servoKeluarGateActive = false;

// Servo objects
inline Servo servoMasuk;
inline Servo servoKeluar;

// ================================================================
//  DETEKSI FISIK via Touch Sensor (ESP32 Hardware Capacitance)
// ================================================================
inline static bool servo_detect_connected(int pin) {
    uint32_t total = 0;
    for (int i = 0; i < SERVO_TOUCH_SAMPLES; i++) {
        total += touchRead(pin);
        delay(8);  // Stabilisasi antar sampel
    }
    uint32_t avg = total / SERVO_TOUCH_SAMPLES;

#if SERVO_DEBUG_TOUCH
    Serial.print("[SERVO_DEBUG] Pin ");
    Serial.print(pin);
    Serial.print(" touchRead avg = ");
    Serial.println(avg);
#endif

    return (avg < SERVO_TOUCH_THRESHOLD);
}

// ================================================================
//  INISIALISASI SERVO — deteksi fisik saat startup
// ================================================================
inline void servo_init() {
    servoMasuk.attach(PIN_SERVO_MASUK);
    servoKeluar.attach(PIN_SERVO_KELUAR);
    delay(100);

    servoMasuk.write(SERVO_CLOSE);
    servoKeluar.write(SERVO_CLOSE);
    servoMasukLastPos  = SERVO_CLOSE;
    servoKeluarLastPos = SERVO_CLOSE;
    delay(400);

    // ── Deteksi servo masuk saat startup ──
    servoMasuk.detach();
    delay(20);  // Beri waktu pin settle setelah lepas PWM
    bool masukOk = servo_detect_connected(PIN_SERVO_MASUK);
    servoMasuk.attach(PIN_SERVO_MASUK);
    delay(30);
    servoMasuk.write(SERVO_CLOSE);

    // ── Deteksi servo keluar saat startup ──
    servoKeluar.detach();
    delay(20);
    bool keluarOk = servo_detect_connected(PIN_SERVO_KELUAR);
    servoKeluar.attach(PIN_SERVO_KELUAR);
    delay(30);
    servoKeluar.write(SERVO_CLOSE);

    servoMasukConnected  = masukOk;
    servoKeluarConnected = keluarOk;

    // Kirim status awal via Serial
    if (masukOk) {
        Serial.println("[SERVO_MASUK] Terhubung");
    } else {
        Serial.println("[SERVO_MASUK] RUSAK - Servo tidak terhubung");
    }

    if (keluarOk) {
        Serial.println("[SERVO_KELUAR] Terhubung");
    } else {
        Serial.println("[SERVO_KELUAR] RUSAK - Servo tidak terhubung");
    }
}

// ================================================================
//  CEK SERVO PERIODIK (setiap 5 detik)
// ================================================================
inline void servo_check_periodic() {
    static unsigned long lastCheck = 0;
    const unsigned long INTERVAL = 5000UL;  // Cek setiap 5 detik

    if ((millis() - lastCheck) < INTERVAL) return;

    // Jangan cek saat palang sedang terbuka
    if (servoMasukGateActive || servoKeluarGateActive) return;

    lastCheck = millis();

    // ────── Cek Servo Masuk ──────
    servoMasuk.detach();
    delay(20);
    bool masukNow = servo_detect_connected(PIN_SERVO_MASUK);
    servoMasuk.attach(PIN_SERVO_MASUK);
    delay(30);
    servoMasuk.write(servoMasukLastPos);

    if (!masukNow && servoMasukConnected) {
        // Baru saja dicabut
        servoMasukConnected = false;
        Serial.println("[SERVO_MASUK] RUSAK - Servo tidak terhubung");
    } else if (masukNow && !servoMasukConnected) {
        // Baru saja dipasang kembali
        servoMasukConnected = true;
        Serial.println("[SERVO_MASUK] KEMBALI Terhubung");
    } else if (masukNow) {
        // Heartbeat normal — terpasang, tidak ada perubahan
        Serial.println("[SERVO_MASUK] Berfungsi normal");
    }

    delay(50);

    // ────── Cek Servo Keluar ──────
    servoKeluar.detach();
    delay(20);
    bool keluarNow = servo_detect_connected(PIN_SERVO_KELUAR);
    servoKeluar.attach(PIN_SERVO_KELUAR);
    delay(30);
    servoKeluar.write(servoKeluarLastPos);

    if (!keluarNow && servoKeluarConnected) {
        servoKeluarConnected = false;
        Serial.println("[SERVO_KELUAR] RUSAK - Servo tidak terhubung");
    } else if (keluarNow && !servoKeluarConnected) {
        servoKeluarConnected = true;
        Serial.println("[SERVO_KELUAR] KEMBALI Terhubung");
    } else if (keluarNow) {
        Serial.println("[SERVO_KELUAR] Berfungsi normal");
    }
}

// ================================================================
//  OPERASI SERVO — update flag & posisi terakhir
// ================================================================
inline void servo_masuk_open() {
    if (servoMasukConnected && servoMasuk.attached()) {
        servoMasukGateActive = true;
        servoMasuk.write(SERVO_OPEN);
        servoMasukLastPos = SERVO_OPEN;
    }
}

inline void servo_masuk_close() {
    if (servoMasuk.attached()) {
        servoMasuk.write(SERVO_CLOSE);
        servoMasukLastPos    = SERVO_CLOSE;
        servoMasukGateActive = false;
    }
}

inline void servo_keluar_open() {
    if (servoKeluarConnected && servoKeluar.attached()) {
        servoKeluarGateActive = true;
        servoKeluar.write(SERVO_OPEN);
        servoKeluarLastPos = SERVO_OPEN;
    }
}

inline void servo_keluar_close() {
    if (servoKeluar.attached()) {
        servoKeluar.write(SERVO_CLOSE);
        servoKeluarLastPos    = SERVO_CLOSE;
        servoKeluarGateActive = false;
    }
}

#endif // SERVO_GATE_H

