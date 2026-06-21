// ============================================================
//  servo_gate.h — Kontrol Servo Palang SG90
//
//  DETEKSI FISIK SERVO via ESP32 Touch Sensor (Hardware Kapasitansi)
//
//  ESP32 memiliki sensor kapasitif bawaan (touch sensor) yang
//  mengukur kapasitansi secara hardware — jauh lebih reliable
//  daripada metode software timing (RC rise time).
//
//  Prinsip kerja:
//    Kabel sinyal servo ~30cm memiliki kapasitansi ~100-300pF
//    (kabel + input CMOS SG90). Kapasitansi ini menurunkan
//    nilai touchRead() secara signifikan vs tanpa servo.
//
//    Servo TERPASANG → touchRead RENDAH  (< SERVO_TOUCH_THRESHOLD)
//    Servo DICABUT   → touchRead TINGGI  (≥ SERVO_TOUCH_THRESHOLD)
//
//  Pin yang support touchRead:
//    GPIO 13 = Touch4 (T4)  → PIN_SERVO_MASUK  ✓
//    GPIO 12 = Touch5 (T5)  → PIN_SERVO_KELUAR ✓
//
//  KALIBRASI THRESHOLD:
//    Aktifkan SERVO_DEBUG_TOUCH = 1 untuk mencetak nilai touchRead
//    ke Serial Monitor. Catat nilai saat servo terpasang dan dicabut,
//    lalu set SERVO_TOUCH_THRESHOLD di antara dua nilai tersebut.
//    Contoh: terpasang=15, dicabut=85 → threshold=50 (default).
// ============================================================
#pragma once
#include <ESP32Servo.h>
#include "config.h"

// ---- Konfigurasi deteksi ----
#define SERVO_TOUCH_THRESHOLD  50   // Ubah jika false positive/negative
#define SERVO_TOUCH_SAMPLES    7    // Jumlah sampel per pengukuran
#define SERVO_DEBUG_TOUCH      0    // Set 1 untuk cetak nilai ke Serial

// ---- Status & posisi servo ----
bool servoMasukConnected  = false;
bool servoKeluarConnected = false;
int  servoMasukLastPos    = SERVO_CLOSE;
int  servoKeluarLastPos   = SERVO_CLOSE;

// Flag: palang sedang terbuka (jangan cek saat gate aktif)
bool servoMasukGateActive  = false;
bool servoKeluarGateActive = false;

// Servo objects
Servo servoMasuk;
Servo servoKeluar;

// ================================================================
//  DETEKSI FISIK via Touch Sensor (ESP32 Hardware Capacitance)
//
//  Harus dipanggil SETELAH servo.detach() agar pin bebas dari PWM.
//  Mengambil rata-rata SERVO_TOUCH_SAMPLES pembacaan untuk noise.
// ================================================================
static bool servo_detect_connected(int pin) {
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

    // Nilai RENDAH = ada kapasitansi kabel = servo TERPASANG
    // Nilai TINGGI = tidak ada kapasitansi  = servo DICABUT
    return (avg < SERVO_TOUCH_THRESHOLD);
}

// ================================================================
//  INISIALISASI SERVO — deteksi fisik saat startup
// ================================================================
void servo_init() {
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

    // Kirim status awal ke Laptop via UART
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
//
//  Mendeteksi perubahan kondisi fisik secara real-time:
//    - Servo terpasang → tiba-tiba dicabut  : kirim RUSAK
//    - Servo dicabut   → tiba-tiba dipasang : kirim KEMBALI Terhubung
//    - Kondisi tidak berubah                : kirim heartbeat
//
//  DILEWATI jika palang sedang terbuka (gate active) untuk
//  mencegah servo drop saat kendaraan lewat.
// ================================================================
void servo_check_periodic() {
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
    // Jika !masukNow && !servoMasukConnected: sudah error, tidak kirim ulang

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

void servo_masuk_open() {
    if (servoMasukConnected && servoMasuk.attached()) {
        servoMasukGateActive = true;
        servoMasuk.write(SERVO_OPEN);
        servoMasukLastPos = SERVO_OPEN;
    }
}

void servo_masuk_close() {
    if (servoMasuk.attached()) {
        servoMasuk.write(SERVO_CLOSE);
        servoMasukLastPos    = SERVO_CLOSE;
        servoMasukGateActive = false;
    }
}

void servo_keluar_open() {
    if (servoKeluarConnected && servoKeluar.attached()) {
        servoKeluarGateActive = true;
        servoKeluar.write(SERVO_OPEN);
        servoKeluarLastPos = SERVO_OPEN;
    }
}

void servo_keluar_close() {
    if (servoKeluar.attached()) {
        servoKeluar.write(SERVO_CLOSE);
        servoKeluarLastPos    = SERVO_CLOSE;
        servoKeluarGateActive = false;
    }
}
