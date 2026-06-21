// ============================================================================
//  Test_Tiga_TOF.ino — Program Uji Coba Mandiri 3 Sensor TOF VL53L1X
//  Sistem ParkEasy ESP32 (A1, A2, B1 via TCA9548A Mux)
//  
//  Digunakan untuk menguji apakah sensor rusak atau tidak secara terisolasi.
//  Menampilkan jarak, status I2C, dan Status Range (kualitas sensor).
// ============================================================================

#include <Wire.h>
#include <VL53L1X.h>

// --- I2C & Multiplexer Configuration ---
#define PIN_SDA         21
#define PIN_SCL         22
#define TCA_ADDR        0x70
#define TOF_I2C_ADDR    0x29

// --- TCA9548A Channels ---
#define TCA_CH_TOF_A1   2    // Slot A1
#define TCA_CH_TOF_A2   3    // Slot A2 (Sensor yang bermasalah)
#define TCA_CH_TOF_B1   4    // Slot B1

#define TOF_COUNT       3
const uint8_t tofChannels[TOF_COUNT] = {TCA_CH_TOF_A1, TCA_CH_TOF_A2, TCA_CH_TOF_B1};
const char* slotNames[TOF_COUNT] = {"A1 (Channel 2)", "A2 (Channel 3)", "B1 (Channel 4)"};

VL53L1X tofSensors[TOF_COUNT];
bool tofConnected[TOF_COUNT] = {false, false, false};

// --- Helper: Pilih Channel TCA9548A ---
void tca_select(uint8_t channel) {
    if (channel > 7) return;
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

// --- Helper: Matikan Semua Channel TCA9548A ---
void tca_disable_all() {
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
}

// --- Helper: Dapatkan Arti Status Range ---
const char* getRangeStatusDesc(uint8_t rangeStatus) {
    switch (rangeStatus) {
        case 0:  return "VALID (Data Oke)";
        case 1:  return "SIGMA_FAIL (Sinyal tidak stabil / noise)";
        case 2:  return "SIGNAL_FAIL (Refleksi terlalu lemah / sensor tertutup)";
        case 4:  return "OUT_OF_BOUNDS (Target di luar jarak maksimum)";
        case 7:  return "WRAP_AROUND (Pantulan ganda / membingungkan)";
        default: return "ERROR_LAIN / UNKNOWN";
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n==================================================");
    Serial.println("   DIAGNOSTIK MANDIRI 3 SENSOR TOF VL53L1X        ");
    Serial.println("==================================================");

    // Inisialisasi I2C
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000); // Kecepatan standar 100 kHz untuk kestabilan

    // ── STEP 1: Scan I2C pada Mux secara detail ──
    Serial.println("\n[1] Memindai Alamat I2C pada setiap Channel Mux...");
    for (uint8_t t = 0; t < 8; t++) {
        tca_select(t);
        delay(10);
        
        Serial.print(" -> Channel ");
        Serial.print(t);
        Serial.print(": ");
        
        int foundCount = 0;
        for (uint8_t addr = 1; addr < 127; addr++) {
            if (addr == TCA_ADDR) continue;
            
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.print("0x");
                if (addr < 16) Serial.print("0");
                Serial.print(addr, HEX);
                Serial.print(" ");
                foundCount++;
            }
        }
        
        if (foundCount == 0) {
            Serial.print("Kosong (Tidak ada perangkat)");
        }
        Serial.println();
    }
    tca_disable_all();

    // ── STEP 2: Inisialisasi Sensor TOF satu per satu ──
    Serial.println("\n[2] Menginisialisasi Sensor VL53L1X...");
    for (int i = 0; i < TOF_COUNT; i++) {
        uint8_t ch = tofChannels[i];
        Serial.print(" -> Menghubungi Slot ");
        Serial.print(slotNames[i]);
        Serial.print("... ");

        tca_select(ch);
        delay(50); // Jeda waktu agar I2C stabil

        uint16_t modelID = tofSensors[i].readReg16Bit(0x010F); // Register IDENTIFICATION__MODEL_ID
        Serial.print("[Model ID: 0x");
        Serial.print(modelID, HEX);
        Serial.print("] -> ");

        tofSensors[i].setTimeout(500);
        
        bool success = false;
        for (int attempt = 1; attempt <= 3; attempt++) {
            if (tofSensors[i].init()) {
                success = true;
                break;
            }
            delay(50);
        }

        if (success) {
            tofConnected[i] = true;
            // Kita coba pakai Short Mode karena lebih stabil pada jarak dekat (< 1.3 meter)
            // Dan lebih tahan terhadap gangguan cahaya sekitar / noise
            tofSensors[i].setDistanceMode(VL53L1X::Short);
            tofSensors[i].setMeasurementTimingBudget(50000); // 50 ms budget
            tofSensors[i].startContinuous(100);             // Ambil data tiap 100 ms
            Serial.println("OK (Ranging: SHORT Mode)");
        } else {
            tofConnected[i] = false;
            Serial.println("GAGAL (Sensor tidak merespon)");
        }
    }
    tca_disable_all();
    
    Serial.println("\n==================================================");
    Serial.println("   Mulai Pengujian Ranging... (Tiap 1 Detik)      ");
    Serial.println("==================================================");
    delay(1000);
}

void loop() {
    Serial.println("--------------------------------------------------");
    
    for (int i = 0; i < TOF_COUNT; i++) {
        Serial.print("Slot ");
        Serial.print(slotNames[i]);
        Serial.print(": ");

        if (!tofConnected[i]) {
            Serial.println("[OFFLINE] Tidak terdeteksi saat booting.");
            continue;
        }

        tca_select(tofChannels[i]);
        delay(10); // Jeda stabilisasi channel I2C sebelum membaca data

        // Membaca jarak secara langsung
        uint16_t distance = tofSensors[i].read(false);
        uint8_t i2c_status = tofSensors[i].last_status;
        uint8_t range_status = tofSensors[i].ranging_data.range_status;

        if (i2c_status == 0) {
            Serial.print("[ONLINE] Jarak = ");
            Serial.print(distance);
            Serial.print(" mm | Range Status = ");
            Serial.print(range_status);
            Serial.print(" - ");
            Serial.print(getRangeStatusDesc(range_status));

            // Evaluasi pembacaan berdasarkan status
            if (range_status != 0) {
                // Jika status bukan 0, data dibuang / diabaikan
                Serial.println(" -> [DATA INVALID]");
            } else {
                if (distance < 150) {
                    Serial.println(" -> [TERDETEKSI DEKAT (DITEMPATI)]");
                } else {
                    Serial.println(" -> [TERDETEKSI JAUH (KOSONG)]");
                }
            }
        } else {
            Serial.print("[I2C ERROR] Kode Status: ");
            Serial.print(i2c_status);
            Serial.println(" (Kabel terputus)");
        }
    }

    tca_disable_all();
    delay(1000); // Uji coba setiap 1 detik
}
