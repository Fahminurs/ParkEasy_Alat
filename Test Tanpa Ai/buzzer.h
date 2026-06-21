// ============================================================
//  buzzer.h — Kontrol Buzzer Gerbang Masuk & Keluar
// ============================================================
#ifndef BUZZER_H
#define BUZZER_H
#pragma once
#include <Arduino.h>
#include "config.h"

// Status buzzer global
inline bool buzzerMasukOk  = false;
inline bool buzzerKeluarOk = false;
inline bool buzzerCheckDone = false;  // Flag: sudah dicek sekali di setup()

// Inisialisasi buzzer sebagai OUTPUT
inline void buzzer_init() {
    pinMode(PIN_BUZZER_MASUK,  OUTPUT);
    pinMode(PIN_BUZZER_KELUAR, OUTPUT);
    digitalWrite(PIN_BUZZER_MASUK,  LOW);
    digitalWrite(PIN_BUZZER_KELUAR, LOW);
}

// ─────────────────────────────────────────
//  Cek koneksi buzzer dengan INPUT_PULLUP
//  LOW  = terhubung (pullup active)
//  HIGH = disconnect / rusak
//
//  PERHATIAN: Hanya dipanggil SEKALI saat setup()
// ─────────────────────────────────────────
inline void buzzer_check_connection() {
    // Jangan cek lagi jika sudah pernah dicek
    if (buzzerCheckDone) return;
    buzzerCheckDone = true;

    bool masukTerhubung  = false;
    bool keluarTerhubung = false;

    // ── Cek Buzzer Masuk ──
    pinMode(PIN_BUZZER_MASUK, INPUT_PULLUP);  // Aktifkan pullup internal
    delayMicroseconds(100);                     // Tunggu sinyal stabil
    int bacaMasuk = digitalRead(PIN_BUZZER_MASUK);
    masukTerhubung = (bacaMasuk == LOW);        // LOW = terhubung ke GND via buzzer

    // ── Cek Buzzer Keluar ──
    pinMode(PIN_BUZZER_KELUAR, INPUT_PULLUP);
    delayMicroseconds(100);
    int bacaKeluar = digitalRead(PIN_BUZZER_KELUAR);
    keluarTerhubung = (bacaKeluar == LOW);

    // ── Kembalikan ke OUTPUT normal ──
    pinMode(PIN_BUZZER_MASUK,  OUTPUT);
    pinMode(PIN_BUZZER_KELUAR, OUTPUT);
    digitalWrite(PIN_BUZZER_MASUK,  LOW);
    digitalWrite(PIN_BUZZER_KELUAR, LOW);

    // ── Update status global ──
    buzzerMasukOk  = masukTerhubung;
    buzzerKeluarOk = keluarTerhubung;

    // ── Kirim status via Serial untuk debug ──
    Serial.print("[BUZZER_MASUK] ");
    Serial.println(masukTerhubung ? "OK" : "ERR");

    Serial.print("[BUZZER_KELUAR] ");
    Serial.println(keluarTerhubung ? "OK" : "ERR");
}

// ─────────────────────────────────────────
//  Cek berkala buzzer (OPSIONAL)
// ─────────────────────────────────────────
static bool lastBuzzerMasukOk  = false;
static bool lastBuzzerKeluarOk = false;
static unsigned long lastBuzzerCheck = 0;
#define BUZZER_CHECK_INTERVAL 2000  // Cek tiap 2 detik (REAL-TIME)

inline void buzzer_check_periodic() {
    unsigned long now = millis();
    if (now - lastBuzzerCheck < BUZZER_CHECK_INTERVAL) return;
    lastBuzzerCheck = now;

    bool masukTerhubung  = false;
    bool keluarTerhubung = false;

    // Cek Masuk
    pinMode(PIN_BUZZER_MASUK, INPUT_PULLUP);
    delayMicroseconds(100);
    int bacaMasuk = digitalRead(PIN_BUZZER_MASUK);
    masukTerhubung = (bacaMasuk == LOW);

    // Cek Keluar
    pinMode(PIN_BUZZER_KELUAR, INPUT_PULLUP);
    delayMicroseconds(100);
    int bacaKeluar = digitalRead(PIN_BUZZER_KELUAR);
    keluarTerhubung = (bacaKeluar == LOW);

    // Kembalikan ke OUTPUT
    pinMode(PIN_BUZZER_MASUK,  OUTPUT);
    pinMode(PIN_BUZZER_KELUAR, OUTPUT);
    digitalWrite(PIN_BUZZER_MASUK,  LOW);
    digitalWrite(PIN_BUZZER_KELUAR, LOW);

    // Update status global
    buzzerMasukOk  = masukTerhubung;
    buzzerKeluarOk = keluarTerhubung;

    // Kirim HANYA jika status BERUBAH
    if (masukTerhubung != lastBuzzerMasukOk) {
        Serial.print("[BUZZER_MASUK] ");
        Serial.println(masukTerhubung ? "OK" : "ERR");
        lastBuzzerMasukOk = masukTerhubung;
    }

    if (keluarTerhubung != lastBuzzerKeluarOk) {
        Serial.print("[BUZZER_KELUAR] ");
        Serial.println(keluarTerhubung ? "OK" : "ERR");
        lastBuzzerKeluarOk = keluarTerhubung;
    }
}

// Beep singkat (proses / gagal)
inline void buzzer_beep_short(int pin) {
    digitalWrite(pin, HIGH);
    delay(BUZZER_SHORT_MS);
    digitalWrite(pin, LOW);
}

// Beep panjang (berhasil / palang terbuka)
inline void buzzer_beep_long(int pin) {
    digitalWrite(pin, HIGH);
    delay(BUZZER_LONG_MS);
    digitalWrite(pin, LOW);
}

// Beep 2x cepat (notifikasi)
inline void buzzer_beep_double(int pin) {
    for (int i = 0; i < 2; i++) {
        digitalWrite(pin, HIGH);
        delay(200);
        digitalWrite(pin, LOW);
        delay(150);
    }
}

#endif // BUZZER_H


