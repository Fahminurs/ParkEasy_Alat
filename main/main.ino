// ============================================================
//  main.ino — Firmware Utama ESP32 ParkEasy
// ============================================================
//
//  Dependensi Library (install via Arduino Library Manager):
//    - ESP32Servo           (kontrol servo SG90)
//    - LiquidCrystal_I2C   (LCD 20x4)
//    - VL53L1X              (sensor TOF, by Pololu)
//
//  Pin Summary (lihat config.h untuk detail):
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
//
//  Komunikasi UART:
//    Laptop → ESP32  : Command "MASUK:BERHASIL\n", "KELUAR:GAGAL\n", dst.
//    ESP32  → Laptop : Update slot "SLOT:A1:DITEMPATI\n", dst.
// ============================================================

#include <Wire.h>
#include "config.h"
#include "buzzer.h"
#include "servo_gate.h"
#include "lcd_display.h"
#include "hcsr04.h"
#include "tof_sensor.h"
#include "uart_handler.h"

// ── Interval update Sensor ─────────────────────────────────────
#define TOF_UPDATE_INTERVAL       500
#define HCSR04_PRINT_INTERVAL     1000   // Print jarak tiap 1 detik
#define HCSR04_HEARTBEAT_INTERVAL 5000   // Heartbeat "Berfungsi normal" tiap 5 detik
#define STATUS_INTERVAL           10000  // Heartbeat semua sensor tiap 10 detik
unsigned long lastTofCheck        = 0;
unsigned long lastHcsr04Print     = 0;
unsigned long lastStatusSend      = 0;
unsigned long lastHcsr04Heartbeat = 0;   // Timer heartbeat periodik HCSR04

// Pelacak error HCSR04 (jumlah baca gagal berturut-turut)
int  hcsr04MasukErrCount  = 0;
int  hcsr04KeluarErrCount = 0;
bool hcsr04MasukError     = false;
bool hcsr04KeluarError    = false;
#define HCSR04_ERR_THRESHOLD 5  // 5x gagal baca berturut-turut = rusak

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
    Serial.setTimeout(50);  // Timeout untuk parse Serial

    Wire.begin(PIN_SDA, PIN_SCL);

    Serial.println("[ParkEasy] Inisialisasi sistem...");

    buzzer_init();
    Serial.println("[OK] Buzzer siap");
    Serial.flush();  // Pastikan pesan terkirim

    servo_init();
    // Status servo (Terhubung/RUSAK) sudah dikirim oleh servo_init()
    Serial.flush();


    lcd_init();
    Serial.println("[OK] LCD siap");
    Serial.flush();

    hcsr04_init();
    Serial.println("[OK] HCSR04 siap");
    Serial.flush();

    tof_init();
    Serial.println("[OK] TOF sensor siap");
    Serial.flush();

    // Tampilkan layar idle di kedua LCD
    lcd_masuk_idle();
    lcd_keluar_idle();

    // Cek koneksi buzzer SEKALI saat startup - kirim hasil via UART
    buzzer_check_connection();
    Serial.flush();  // Pastikan status buzzer terkirim

    // Beep singkat tanda sistem siap (hanya jika buzzer terhubung)
    buzzer_beep_double(PIN_BUZZER_MASUK);
    delay(200);
    buzzer_beep_double(PIN_BUZZER_KELUAR);

    Serial.println("[ParkEasy] Sistem siap. Menunggu kendaraan...");
    Serial.flush();
}


// ================================================================
//  LOOP
// ================================================================
void loop() {
    // 0. Baca command dari Laptop via UART
    uart_read();

    // 1. Cek koneksi buzzer berkala (tiap 30 detik, hanya jika berubah)
    buzzer_check_periodic();

    // 2. Update sensor TOF setiap TOF_UPDATE_INTERVAL ms
    unsigned long now = millis();
    if (now - lastTofCheck >= TOF_UPDATE_INTERVAL) {
        lastTofCheck = now;
        tof_update();
    }

    // ================================================================
    //  3. CEK HCSR04 SETIAP 1 DETIK
    //  Pakai counter untuk mencegah false toggle
    // ================================================================
    static unsigned long lastHcsr04Check = 0;
    if (now - lastHcsr04Check >= 1000) {
        lastHcsr04Check = now;

        // --- Baca jarak HCSR04 Masuk ---
        float distMasuk = hcsr04_read_cm(PIN_TRIG1, PIN_ECHO1);

        // Kondisi sensor normal: jarak valid (2-400cm)
        bool masukValid = (distMasuk >= 2.0f && distMasuk <= 400.0f);

        if (masukValid) {
            // Sensor membaca dengan baik
            hcsr04MasukErrCount = 0;
            if (hcsr04MasukError) {
                // Baru pulih setelah error — kirim status pulih langsung
                hcsr04MasukError = false;
                Serial.println("[HCSR04_MASUK] KEMBALI Terhubung");
                Serial.flush();
            }
            // Print jarak
            Serial.print("[HCSR04_MASUK] ");
            Serial.println(distMasuk);
        } else {
            // Gagal baca (timeout atau jarak tidak valid)
            hcsr04MasukErrCount++;
            if (hcsr04MasukErrCount >= HCSR04_ERR_THRESHOLD && !hcsr04MasukError) {
                hcsr04MasukError = true;
                Serial.println("[HCSR04_MASUK] RUSAK - Gagal membaca sensor");
                Serial.flush();
            }
        }

        // --- Baca jarak HCSR04 Keluar ---
        float distKeluar = hcsr04_read_cm(PIN_TRIG2, PIN_ECHO2);

        bool keluarValid = (distKeluar >= 2.0f && distKeluar <= 400.0f);

        if (keluarValid) {
            hcsr04KeluarErrCount = 0;
            if (hcsr04KeluarError) {
                hcsr04KeluarError = false;
                Serial.println("[HCSR04_KELUAR] KEMBALI Terhubung");
                Serial.flush();
            }
            Serial.print("[HCSR04_KELUAR] ");
            Serial.println(distKeluar);
        } else {
            hcsr04KeluarErrCount++;
            if (hcsr04KeluarErrCount >= HCSR04_ERR_THRESHOLD && !hcsr04KeluarError) {
                hcsr04KeluarError = true;
                Serial.println("[HCSR04_KELUAR] RUSAK - Gagal membaca sensor");
                Serial.flush();
            }
        }
    }

    // ================================================================
    //  3b. HEARTBEAT HCSR04 SETIAP 5 DETIK (hanya saat sensor normal)
    //  Persis pola servo: tidak kirim jika sedang error
    // ================================================================
    if (now - lastHcsr04Heartbeat >= HCSR04_HEARTBEAT_INTERVAL) {
        lastHcsr04Heartbeat = now;

        if (!hcsr04MasukError) {
            Serial.println("[HCSR04_MASUK] Berfungsi normal");
            Serial.flush();
        }
        // Jika hcsr04MasukError: diam — sudah dikirim pesan RUSAK sebelumnya

        if (!hcsr04KeluarError) {
            Serial.println("[HCSR04_KELUAR] Berfungsi normal");
            Serial.flush();
        }
        // Jika hcsr04KeluarError: diam — sudah dikirim pesan RUSAK sebelumnya
    }

    // ================================================================
    //  4. CEK SERVO PERIODIK (setiap 5 detik)
    //  Deteksi servo terputus/rusak secara realtime
    // ================================================================
    servo_check_periodic();

    // 5. Heartbeat semua sensor (tiap 10 detik)
    if (now - lastStatusSend >= STATUS_INTERVAL) {
        lastStatusSend = now;

        // Cek status LCD Masuk
        if (!lcd_masuk_check()) {
            if (!lcdMasukError) {
                lcdMasukError = true;
                Serial.println("[LCD_MASUK] ERR");
                Serial.flush();
            }
        } else {
            if (lcdMasukError) {
                lcdMasukError = false;
                Serial.println("[LCD_MASUK] OK");
                Serial.flush();
                // LCD perlu di-init ulang jika habis mati
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
                Serial.flush();
            }
        } else {
            if (lcdKeluarError) {
                lcdKeluarError = false;
                Serial.println("[LCD_KELUAR] OK");
                Serial.flush();
                lcd_init();
                lcd_masuk_idle();
                lcd_keluar_idle();
            }
        }

        Serial.println("[HEARTBEAT] sistem aktif");
        Serial.flush();
    }

    delay(50); // stabilisasi loop
}
