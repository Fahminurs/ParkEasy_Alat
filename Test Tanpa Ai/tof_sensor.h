// ============================================================
//  tof_sensor.h — Sensor TOF VL53L1X via TCA9548A
//  Dengan deteksi real-time: koneksi hilang / kembali
// ============================================================
#ifndef TOF_SENSOR_H
#define TOF_SENSOR_H
#pragma once
#include <Wire.h>
#include <VL53L1X.h>
#include "config.h"

// Slot: A1=Ch2, A2=Ch3, B1=Ch4
#define TOF_COUNT 3
static const uint8_t tofChannels[TOF_COUNT] = {TCA_CH_TOF_A1, TCA_CH_TOF_A2, TCA_CH_TOF_B1};
static const char*   slotCodes[TOF_COUNT]   = {"A1", "A2", "B1"};

inline VL53L1X      tofSensors[TOF_COUNT];
inline bool         tofOccupied[TOF_COUNT]  = {false, false, false};
inline bool         tofConnected[TOF_COUNT] = {false, false, false};
inline unsigned long tofLastSeen[TOF_COUNT] = {0, 0, 0};

#define TOF_TIMEOUT_MS      5000   // jika tidak ada data > 5 detik → error
#define TOF_RECONNECT_MS    3000   // coba reconnect tiap 3 detik

inline unsigned long lastReconnectCheck = 0;

extern void tca_select(uint8_t channel);


// ── Helper: inisialisasi 1 sensor ────────────────────────────
inline bool tof_init_one(int i) {
    tca_select(tofChannels[i]);
    delay(20); // Jeda stabilisasi jalur I2C setelah perpindahan channel TCA
    tofSensors[i].setTimeout(500);
    
    // Coba inisialisasi sensor hingga 3 kali jika gagal
    bool success = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (tofSensors[i].init()) {
            success = true;
            break;
        }
        delay(50);
    }
    
    if (!success) return false;

    tofSensors[i].setDistanceMode(VL53L1X::Short); // Menggunakan mode jarak pendek untuk stabilitas jarak dekat (< 1.3m)
    tofSensors[i].setMeasurementTimingBudget(50000);
    tofSensors[i].startContinuous(100);
    return true;
}


// ── Inisialisasi semua sensor (dipanggil di setup) ────────────
inline void tof_init() {
    for (int i = 0; i < TOF_COUNT; i++) {
        if (tof_init_one(i)) {
            tofConnected[i] = true;
            tofLastSeen[i]  = millis();
        } else {
            tofConnected[i] = false;
            Serial.print("[TOF] Gagal init sensor slot ");
            Serial.println(slotCodes[i]);
        }
    }
    Serial.println("[OK] TOF sensor siap");
}


// ── Update loop (dipanggil dari loop() tiap 500ms) ────────────
inline void tof_update() {
    unsigned long now = millis();

    // ── 1. Coba reconnect sensor yang terputus ────────────────
    if (now - lastReconnectCheck >= TOF_RECONNECT_MS) {
        lastReconnectCheck = now;
        for (int i = 0; i < TOF_COUNT; i++) {
            if (!tofConnected[i]) {
                if (tof_init_one(i)) {
                    tofConnected[i] = true;
                    tofLastSeen[i]  = now;
                    Serial.print("[TOF] OK sensor slot ");
                    Serial.println(slotCodes[i]);
                }
            }
        }
    }

    // ── 2. Baca data dari sensor yang terhubung ───────────────
    for (int i = 0; i < TOF_COUNT; i++) {
        if (!tofConnected[i]) continue;

        tca_select(tofChannels[i]);
        delay(5); // Beri jeda agar bus I2C stabil setelah perpindahan channel TCA

        uint16_t dist_mm = tofSensors[i].read(false);

        // Periksa apakah sensor merespon transmisi I2C secara fisik
        if (tofSensors[i].last_status == 0) {
            tofLastSeen[i] = now; // Sensor terhubung fisik

            // Proses data jika nilainya valid
            if (dist_mm > 0 && dist_mm < 6000) {
                bool occupied = (dist_mm < TOF_OCCUPIED_MM);
                if (occupied != tofOccupied[i]) {
                    tofOccupied[i] = occupied;
                    Serial.print("SLOT:");
                    Serial.print(slotCodes[i]);
                    Serial.print(":");
                    Serial.println(occupied ? "DITEMPATI" : "TERSEDIA");
                }
            }
        } else {
            // ── Transmisi I2C gagal (kabel lepas/rusak) → cek timeout ──────────────────
            if (now - tofLastSeen[i] > TOF_TIMEOUT_MS) {
                tofConnected[i] = false;
                Serial.print("[TOF] Gagal sensor slot ");
                Serial.println(slotCodes[i]);
            }
        }
    }
}

#endif // TOF_SENSOR_H

