# ESP32 Pin Configuration - ParkEasy

> Dokumentasi pinout ESP32 untuk sistem ParkEasy
> Dibuat: 2026-05-15
> ESP32 Board: DevKit atau serupa

---

## Daftar Pin GPIO

| Pin | Komponen | Fungsi | Mode |
|-----|---------|--------|------|
| **GPIO 18** | Buzzer | Buzzer Gerbang Masuk | OUTPUT |
| **GPIO 19** | Buzzer | Buzzer Gerbang Keluar | OUTPUT |
| **GPIO 13** | Servo SG90 | Palang Gerbang Masuk | PWM |
| **GPIO 12** | Servo SG90 | Palang Gerbang Keluar | PWM |
| **GPIO 21** | I2C (SDA) | Komunikasi TCA9548A | INPUT_PULLUP |
| **GPIO 22** | I2C (SCL) | Komunikasi TCA9548A | INPUT_PULLUP |
| **GPIO 14** | HCSR04 | TRIG Gerbang Masuk | OUTPUT |
| **GPIO 27** | HCSR04 | ECHO Gerbang Masuk | INPUT |
| **GPIO 26** | HCSR04 | TRIG Gerbang Keluar | OUTPUT |
| **GPIO 25** | HCSR04 | ECHO Gerbang Keluar | INPUT |

---

## Ringkasan Penggunaan Pin

```
Total GPIO terpakai: 10 pin

Buzzer (2 pin)
├── GPIO 18 → Buzzer Gerbang Masuk
└── GPIO 19 → Buzzer Gerbang Keluar

Servo (2 pin)
├── GPIO 13 → Servo Palang Masuk
└── GPIO 12 → Servo Palang Keluar

HCSR04 Sensor Jarak (4 pin)
├── GPIO 14 → TRIG Masuk
├── GPIO 27 → ECHO Masuk
├── GPIO 26 → TRIG Keluar
└── GPIO 25 → ECHO Keluar

I2C Bus (2 pin)
├── GPIO 21 → SDA
└── GPIO 22 → SCL
```

---

## I2C Devices - TCA9548A Multiplexer

TCA9548A menggunakan alamat `0x70` untuk menghubungkan multiple devices ke bus I2C yang sama.

| Channel TCA | Device | Alamat I2C |
|------------|--------|------------|
| Ch 0 | LCD 2004 Gerbang Masuk | 0x27 |
| Ch 1 | LCD 2004 Gerbang Keluar | 0x27 |
| Ch 2 | VL53L1X Slot A1 | 0x29 |
| Ch 3 | VL53L1X Slot A2 | 0x29 |
| Ch 4 | VL53L1X Slot B1 | 0x29 |
| Ch 5 | VL53L1X Slot B2 | 0x29 |

```
ESP32          TCA9548A
┌────────┐     ┌─────────┐
│ GPIO21 │────►│   SDA   │
│ GPIO22 │────►│   SCL   │
│        │     │         │
│        │     │ CH0 ────┼──────► LCD Masuk (0x27)
│        │     │ CH1 ────┼──────► LCD Keluar (0x27)
│        │     │ CH2 ────┼──────► TOF A1 (0x29)
│        │     │ CH3 ────┼──────► TOF A2 (0x29)
│        │     │ CH4 ────┼──────► TOF B1 (0x29)
│        │     │ CH5 ────┼──────► TOF B2 (0x29)
└────────┘     └─────────┘
```

---

## Status Pin Default (saat booting)

| Pin | Komponen | Status Awal |
|-----|----------|-------------|
| GPIO 18 | Buzzer Masuk | LOW (mati) |
| GPIO 19 | Buzzer Keluar | LOW (mati) |
| GPIO 13 | Servo Masuk | 0 derajat (tertutup) |
| GPIO 12 | Servo Keluar | 0 derajat (tertutup) |
| GPIO 14 | HCSR04 TRIG Masuk | LOW |
| GPIO 27 | HCSR04 ECHO Masuk | INPUT |
| GPIO 26 | HCSR04 TRIG Keluar | LOW |
| GPIO 25 | HCSR04 ECHO Keluar | INPUT |
| GPIO 21 | I2C SDA | INPUT_PULLUP |
| GPIO 22 | I2C SCL | INPUT_PULLUP |

---

## Konfigurasi Serial

| Parameter | Nilai |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Stop Bits | 1 |
| Parity | None |

---

## Format Pesan UART

### ESP32 → Python (Laptop)

```
[BUZZER_MASUK]  OK / ERR
[BUZZER_KELUAR] OK / ERR
[HCSR04_MASUK]  <jarak> cm
[HCSR04_KELUAR] <jarak> cm
[LCD_MASUK]     OK / ERR
[LCD_KELUAR]    OK / ERR
[HEARTBEAT]     sistem aktif
SLOT:<kode>:<status>
MASUK:SELESAI|<plat>
KELUAR:SELESAI|<plat>
```

### Python (Laptop) → ESP32

```
MASUK:BERHASIL|<plat>|<waktu>
MASUK:GAGAL|<plat>|<alasan>
MASUK:DARURAT|<plat>|<waktu>
KELUAR:BERHASIL|<plat>
KELUAR:BELUM_BAYAR|<plat>|<harga>
KELUAR:GAGAL|<plat>|<alasan>
KELUAR:DARURAT|<plat>
ANIM_KAMERA:MASUK
ANIM_KAMERA:KELUAR
ANIM_DATABASE:MASUK
ANIM_DATABASE:KELUAR
reset
```

---

## Konfigurasi Servo

| Parameter | Nilai |
|-----------|-------|
| Frekuensi PWM | 50 Hz |
| Sudut Terbuka | 90 derajat |
| Sudut Tertutup | 0 derajat |
| Durasi Palang Terbuka | 5 detik |

---

## Konfigurasi Sensor HCSR04

| Parameter | Nilai |
|-----------|-------|
| Jarak Deteksi | < 10 cm |
| Max Jarak | 400 cm |
| Error Threshold | 5x pembacaan invalid berturut-turut |
| Interval Print | 1 detik |

---

## Konfigurasi Sensor TOF VL53L1X

| Parameter | Nilai |
|-----------|-------|
| Threshold Occupied | < 150 mm |
| Threshold Empty | >= 200 mm |
| Interval Update | 500 ms |

---

## Catatan Penting

1. **I2C Address 0x27** digunakan oleh LCD 2004 untuk kedua gerbang (masuk dan keluar) melalui multiplexer TCA9548A dengan channel berbeda.

2. **I2C Address 0x29** digunakan oleh VL53L1X untuk 4 slot parking (A1, A2, B1, B2) melalui multiplexer TCA9548A dengan channel berbeda.

3. **Buzzer** bekerja dengan logika active-high (HIGH = berbunyi).

4. **HCSR04** hanya mengirim status ERROR/OK saat terjadi perubahan status, tidak setiap pembacaan.

---

## Update History

| Tanggal | Perubahan |
|--------|-----------|
| 2026-05-15 | Initial documentation - buzzer connection check |