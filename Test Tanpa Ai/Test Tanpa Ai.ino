// ============================================================
//  Test Tanpa Ai.ino — Firmware Uji Coba Otonom (Tanpa AI / UART)
// ============================================================
//
//  Deskripsi:
//    Membuka gerbang secara otomatis jika sensor HC-SR05/HC-SR04
//    mendeteksi kendaraan dalam jarak kurang dari 5 cm.
//    Tanpa memerlukan komunikasi data dari laptop/Python AI.
//
//  Pin Summary (config.h):
//    Pin 18  → Buzzer Gerbang Masuk
//    Pin 19  → Buzzer Gerbang Keluar
//    Pin 13  → Servo Masuk (SG90)
//    Pin 12  → Servo Keluar (SG90)
//    Pin 21  → SDA (TCA9548A)
//    Pin 22  → SCL (TCA9548A)
//    Pin 14  → TRIG HCSR04 Masuk
//    Pin 27  → ECHO HCSR04 Masuk
//    Pin 26  → TRIG HCSR04 Keluar
//    Pin 25  → ECHO HCSR04 Keluar
// ============================================================

#include <Wire.h>
#include "config.h"
#include "buzzer.h"
#include "servo_gate.h"
#include "lcd_display.h"
#include "hcsr04.h"
#include "tof_sensor.h"

// ── Interval update Sensor ─────────────────────────────────────
#define TOF_UPDATE_INTERVAL       500
#define STATUS_INTERVAL           10000  // Heartbeat semua sensor tiap 10 detik

unsigned long lastTofCheck        = 0;
unsigned long lastStatusSend      = 0;

// Pelacak status LCD
bool lcdMasukError  = false;
bool lcdKeluarError = false;

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(SERIAL_BAUD);

    // Tunggu Serial terhubung
    while (!Serial && millis() < 3000);
    
    Wire.begin(PIN_SDA, PIN_SCL);

    Serial.println("[Test Tanpa AI] Inisialisasi sistem otonom...");

    buzzer_init();
    Serial.println("[OK] Buzzer siap");

    servo_init();
    Serial.println("[OK] Servo siap");

    lcd_init();
    Serial.println("[OK] LCD siap");

    hcsr04_init();
    Serial.println("[OK] HCSR04 siap");

    tof_init();
    Serial.println("[OK] TOF sensor siap");

    // Tampilkan layar idle di kedua LCD
    lcd_masuk_idle();
    lcd_keluar_idle();

    // Cek koneksi buzzer
    buzzer_check_connection();

    // Beep singkat tanda sistem siap
    buzzer_beep_double(PIN_BUZZER_MASUK);
    delay(200);
    buzzer_beep_double(PIN_BUZZER_KELUAR);

    Serial.println("[Test Tanpa AI] Sistem siap. Menunggu kendaraan (< 5 cm)...");
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
    unsigned long now = millis();

    // 1. Cek koneksi buzzer berkala (tiap 2 detik, hanya jika berubah)
    buzzer_check_periodic();

    // 2. Update sensor TOF (keterisian slot)
    if (now - lastTofCheck >= TOF_UPDATE_INTERVAL) {
        lastTofCheck = now;
        tof_update();
    }

    // ================================================================
    //  3. CEK DAN DETEKSI KENDARAAN VIA HC-SR04/05
    // ================================================================
    
    // --- Gerbang Masuk ---
    float distMasuk = hcsr04_read_cm(PIN_TRIG1, PIN_ECHO1);
    
    // Debug print jarak ke Serial Monitor tiap 1 detik
    static unsigned long lastDebugPrint = 0;
    if (now - lastDebugPrint >= 1000) {
        lastDebugPrint = now;
        Serial.print("[Jarak] Masuk: ");
        if (distMasuk > 0) {
            Serial.print(distMasuk);
            Serial.print(" cm | ");
        } else {
            Serial.print("Timeout/Disconnect | ");
        }
        
        float distKeluarDebug = hcsr04_read_cm(PIN_TRIG2, PIN_ECHO2);
        Serial.print("Keluar: ");
        if (distKeluarDebug > 0) {
            Serial.print(distKeluarDebug);
            Serial.println(" cm");
        } else {
            Serial.println("Timeout/Disconnect");
        }
    }

    // Jika kendaraan terdeteksi kurang dari 5 cm di Gerbang Masuk
    if (distMasuk > 0.0f && distMasuk < 5.0f) {
        Serial.println("[DETEKSI] Kendaraan terdeteksi masuk (< 5 cm). Memulai proses gerbang masuk...");

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
        
        // Buka Servo Gerbang Masuk ke 90 derajat
        servo_masuk_open();
        
        // Tampilkan pesan sukses seperti pada /main (dengan plat default)
        lcd_masuk_sukses("B 1234 OK", "12:00:00");

        // Tetap buka selama kendaraan masih menghalangi sensor (< 10 cm)
        delay(1000); // Beri jeda mobil untuk maju sedikit
        while (hcsr04_masuk_detected()) {
            delay(100);
        }
        
        // Jeda aman setelah kendaraan lewat sebelum menutup
        delay(2000);
        servo_masuk_close();
        delay(1000);
        lcd_masuk_idle();
        
        Serial.println("[DETEKSI] Gerbang masuk ditutup kembali.");
    }

    // --- Gerbang Keluar ---
    float distKeluar = hcsr04_read_cm(PIN_TRIG2, PIN_ECHO2);

    // Jika kendaraan terdeteksi kurang dari 5 cm di Gerbang Keluar
    if (distKeluar > 0.0f && distKeluar < 5.0f) {
        Serial.println("[DETEKSI] Kendaraan terdeteksi keluar (< 5 cm). Memulai proses gerbang keluar...");

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
        
        // Buka Servo Gerbang Keluar ke 90 derajat
        servo_keluar_open();
        
        // Tampilkan pesan sukses seperti pada /main (dengan plat default)
        lcd_keluar_sukses("B 1234 OK");

        // Tetap buka selama kendaraan masih menghalangi sensor (< 10 cm)
        delay(1000); // Jeda mobil untuk maju sedikit
        while (hcsr04_keluar_detected()) {
            delay(100);
        }
        
        // Jeda aman setelah kendaraan lewat sebelum menutup
        delay(2000);
        servo_keluar_close();
        delay(1000);
        lcd_keluar_idle();
        
        Serial.println("[DETEKSI] Gerbang keluar ditutup kembali.");
    }

    // 4. Cek Servo Periodik (setiap 5 detik)
    servo_check_periodic();

    // 5. Heartbeat dan Cek LCD (tiap 10 detik)
    if (now - lastStatusSend >= STATUS_INTERVAL) {
        lastStatusSend = now;

        // Cek status LCD Masuk
        if (!lcd_masuk_check()) {
            if (!lcdMasukError) {
                lcdMasukError = true;
                Serial.println("[LCD_MASUK] ERR");
            }
        } else {
            if (lcdMasukError) {
                lcdMasukError = false;
                Serial.println("[LCD_MASUK] OK");
                lcd_init();
                lcd_masuk_idle();
                lcd_keluar_idle();
            }
        }

        // Cek status LCD Keluar
        if (!lcd_keluar_check()) {
            if (!lcdKeluarError) {
                lcdKeluarError = true;
                Serial.println("[LCD_KELUAR] ERR");
            }
        } else {
            if (lcdKeluarError) {
                lcdKeluarError = false;
                Serial.println("[LCD_KELUAR] OK");
                lcd_init();
                lcd_masuk_idle();
                lcd_keluar_idle();
            }
        }

        Serial.println("[HEARTBEAT] Sistem aktif & memantau...");
    }

    delay(50); // stabilisasi loop
}
