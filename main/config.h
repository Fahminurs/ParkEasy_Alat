// ============================================================
//  config.h — Definisi Pin & Konstanta Sistem ParkEasy ESP32
// ============================================================
#pragma once

// --- Buzzer ---
#define PIN_BUZZER_MASUK    18   // Buzzer Gerbang Masuk
#define PIN_BUZZER_KELUAR   19   // Buzzer Gerbang Keluar

// --- Servo Palang ---
#define PIN_SERVO_MASUK     13   // Servo SG90 Gerbang Masuk
#define PIN_SERVO_KELUAR    12   // Servo SG90 Gerbang Keluar

// --- I2C (TCA9548A) ---
#define PIN_SDA             21
#define PIN_SCL             22

// --- HCSR04 Gerbang Masuk ---
#define PIN_TRIG1           14
#define PIN_ECHO1           27

// --- HCSR04 Gerbang Keluar ---
#define PIN_TRIG2           26
#define PIN_ECHO2           25

// --- TCA9548A Channel ---
#define TCA_CH_LCD_MASUK    0    // LCD 20x4 Gerbang Masuk  (0x27)
#define TCA_CH_LCD_KELUAR   1    // LCD 20x4 Gerbang Keluar (0x27)
#define TCA_CH_TOF_A1       2    // Sensor TOF VL53L1X slot A1 (0x29)
#define TCA_CH_TOF_A2     3
#define TCA_CH_TOF_B1     4
// Tambahkan channel 3,4,5 jika memakai lebih dari 1 sensor TOF

// --- Alamat I2C ---
#define I2C_ADDR_LCD        0x27
#define I2C_ADDR_TOF        0x29
#define TCA_ADDR            0x70

// --- Servo Sudut ---
#define SERVO_OPEN          90   // derajat palang terbuka
#define SERVO_CLOSE         0    // derajat palang tertutup

// --- Jarak HCSR04 (cm) ---
#define DETECT_DISTANCE_CM  10   // kendaraan terdeteksi jika jarak < nilai ini

// --- TOF Threshold (mm) ---
#define TOF_OCCUPIED_MM     150  // < nilai ini = kendaraan ada di slot
#define TOF_EMPTY_MM        200  // >= nilai ini = slot kosong

// --- Timing ---
#define BUZZER_SHORT_MS     500  // beep pendek (gagal/proses)
#define BUZZER_LONG_MS      2000 // beep panjang (berhasil)
#define GATE_OPEN_MS        5000 // waktu palang terbuka sebelum dicek lagi
#define SERIAL_BAUD         115200
