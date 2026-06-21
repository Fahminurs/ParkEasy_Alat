// ============================================================
//  uart_handler.h — Parsing Command UART dari Laptop
// ============================================================
//  Format Baru Pemisah | :
//    ANIM:MASUK
//    MASUK:BERHASIL|D 123 DX|14:30:00
//    MASUK:GAGAL|D 123 DX
//    ANIM:KELUAR
//    KELUAR:BERHASIL|D 123 DX
//    KELUAR:BELUM_BAYAR|D 123 DX|5000
//    KELUAR:GAGAL|D 123 DX
// ============================================================
#pragma once
#include <Arduino.h>
#include "config.h"
#include "buzzer.h"
#include "servo_gate.h"
#include "lcd_display.h"
#include "hcsr04.h"

static String uartBuffer = "";

// ── Helper Pemisah String (Split) ────────────────────────────
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


// ── Animasi Triggered dari Python ──────────────────────────────
void action_animasi_kamera(int isMasuk) {
    lcd_anim_kamera(isMasuk);
    int buzzerPin = isMasuk ? PIN_BUZZER_MASUK : PIN_BUZZER_KELUAR;
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
}

void action_animasi_database(int isMasuk) {
    lcd_anim_database(isMasuk);
    int buzzerPin = isMasuk ? PIN_BUZZER_MASUK : PIN_BUZZER_KELUAR;
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
}


// ── Parse & Dispatch Command ─────────────────────────────────
void uart_dispatch(const String& cmd) {
    Serial.print("[ESP32] Command diterima: ");
    Serial.println(cmd);

    if (cmd.equalsIgnoreCase("reset")) {
        Serial.println("Mereset ESP32...");
        lcd_reset_system();
        delay(1200);
        ESP.restart();
        return;
    }

    // 1. Ambil action type (contoh: "MASUK:BERHASIL")
    String action = getValue(cmd, '|', 0);

    // 2. Animasi Kamera
    if (action == "ANIM_KAMERA:MASUK") {
        action_animasi_kamera(1);
        return;
    } 
    else if (action == "ANIM_KAMERA:KELUAR") {
        action_animasi_kamera(0);
        return;
    }
    
    // 2.5 Animasi Database
    else if (action == "ANIM_DATABASE:MASUK") {
        action_animasi_database(1);
        return;
    } 
    else if (action == "ANIM_DATABASE:KELUAR") {
        action_animasi_database(0);
        return;
    }

    // 3. Masuk Berhasil
    if (action == "MASUK:BERHASIL" || action == "MASUK:DARURAT") {
        String plate = getValue(cmd, '|', 1);
        String timeStr = getValue(cmd, '|', 2);
        
        // --- Animasi 100% Sukses ---
        lcd_anim_sukses_100(1);
        digitalWrite(PIN_BUZZER_MASUK, HIGH);
        delay(500);
        digitalWrite(PIN_BUZZER_MASUK, LOW);
        delay(1000); // Tahan layar 100% sebentar
        
        // Buzzer 2 detik pembukaan gerbang
        digitalWrite(PIN_BUZZER_MASUK, HIGH);
        delay(2000);
        digitalWrite(PIN_BUZZER_MASUK, LOW);
        
        servo_masuk_open();
        if (action == "MASUK:DARURAT") {
            lcd_masuk_darurat(plate.c_str(), timeStr.c_str());
        } else {
            lcd_masuk_sukses(plate.c_str(), timeStr.c_str());
        }

        // Tetap buka selama kendaraan terdeteksi oleh HCSR04
        delay(1000); // Beri jeda mobil untuk maju sedikit
        while (hcsr04_masuk_detected()) {
            delay(100);
        }
        
        // Jika sudah melewati 2 detik dan tidak terdeteksi
        delay(2000);
        servo_masuk_close();
        delay(1000);
        lcd_masuk_idle();
        
        // Beritahu Laptop bahwa mobil sudah benar-benar masuk
        Serial.print("MASUK:SELESAI|");
        Serial.println(plate);
    }
    // 4. Masuk Gagal
    else if (action == "MASUK:GAGAL") {
        String plate = getValue(cmd, '|', 1);
        String reason = getValue(cmd, '|', 2);
        if (reason == "") reason = "Sistem Error!       ";
        buzzer_beep_short(PIN_BUZZER_MASUK);
        lcd_masuk_gagal(plate.c_str(), reason.c_str());
        delay(3000);
        lcd_masuk_idle();
    }
    // 4.5 Masuk Tidak Terdeteksi
    else if (action == "MASUK:TIDAK_TERDETEKSI") {
        buzzer_beep_short(PIN_BUZZER_MASUK);
        lcd_masuk_tidak_terdeteksi();
        delay(3000);
        lcd_masuk_idle();
    }
    // 5. Keluar Berhasil
    else if (action == "KELUAR:BERHASIL" || action == "KELUAR:DARURAT") {
        String plate = getValue(cmd, '|', 1);
        
        // --- Animasi 100% Sukses ---
        lcd_anim_sukses_100(0);
        digitalWrite(PIN_BUZZER_KELUAR, HIGH);
        delay(500);
        digitalWrite(PIN_BUZZER_KELUAR, LOW);
        delay(1000); // Tahan layar 100% sebentar
        
        // Buzzer 2 detik pembukaan gerbang
        digitalWrite(PIN_BUZZER_KELUAR, HIGH);
        delay(2000);
        digitalWrite(PIN_BUZZER_KELUAR, LOW);
        
        servo_keluar_open();
        if (action == "KELUAR:DARURAT") {
            lcd_keluar_darurat(plate.c_str());
        } else {
            lcd_keluar_sukses(plate.c_str());
        }

        // Tetap buka selama kendaraan terdeteksi oleh HCSR04
        delay(1000); 
        while (hcsr04_keluar_detected()) {
            delay(100);
        }
        
        // Jika sudah melewati 2 detik dan tidak terdeteksi
        delay(2000);
        servo_keluar_close();
        delay(1000);
        lcd_keluar_idle();
        
        // Beritahu Laptop bahwa mobil sudah benar-benar keluar
        Serial.print("KELUAR:SELESAI|");
        Serial.println(plate);
    }
    // 6. Keluar Belum Bayar
    else if (action == "KELUAR:BELUM_BAYAR") {
        String plate = getValue(cmd, '|', 1);
        String price = getValue(cmd, '|', 2);
        buzzer_beep_short(PIN_BUZZER_KELUAR);
        lcd_keluar_belum_bayar(plate.c_str(), price.c_str());
        delay(4000);
        lcd_keluar_idle();
    }
    // 7. Keluar Gagal
    else if (action == "KELUAR:GAGAL") {
        String plate = getValue(cmd, '|', 1);
        String reason = getValue(cmd, '|', 2);
        if (reason == "") reason = "Sistem Error!       ";
        buzzer_beep_short(PIN_BUZZER_KELUAR);
        lcd_keluar_gagal(plate.c_str(), reason.c_str());
        delay(3000);
        lcd_keluar_idle();
    }
    // 7.5 Keluar Tidak Terdeteksi
    else if (action == "KELUAR:TIDAK_TERDETEKSI") {
        buzzer_beep_short(PIN_BUZZER_KELUAR);
        lcd_keluar_tidak_terdeteksi();
        delay(3000);
        lcd_keluar_idle();
    }
}


// ── Baca Serial (dipanggil dari loop()) ──────────────────────
void uart_read() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n') {
            uartBuffer.trim();
            if (uartBuffer.length() > 0) {
                uart_dispatch(uartBuffer);
            }
            uartBuffer = "";
        } else {
            uartBuffer += c;
        }
    }
}
