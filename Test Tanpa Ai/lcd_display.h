// ============================================================
//  lcd_display.h — Kontrol LCD 20x4 via TCA9548A I2C Mux
// ============================================================
#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H
#pragma once
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

// Dua objek LCD, keduanya di alamat 0x27 tapi channel TCA berbeda
inline LiquidCrystal_I2C lcdMasuk (I2C_ADDR_LCD, 20, 4);
inline LiquidCrystal_I2C lcdKeluar(I2C_ADDR_LCD, 20, 4);


// ── TCA9548A: pilih channel ──────────────────────────────────
inline void tca_select(uint8_t channel) {
    if (channel > 7) return;
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

inline void tca_disable_all() {
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
}


// ── Inisialisasi ─────────────────────────────────────────────
inline void lcd_init() {
    tca_select(TCA_CH_LCD_MASUK);
    lcdMasuk.init();
    lcdMasuk.backlight();
    lcdMasuk.clear();

    tca_select(TCA_CH_LCD_KELUAR);
    lcdKeluar.init();
    lcdKeluar.backlight();
    lcdKeluar.clear();

    tca_disable_all();
}

// ── Cek Koneksi Realtime (I2C Ping) ──────────────────────────
inline bool lcd_masuk_check() {
    tca_select(TCA_CH_LCD_MASUK);
    Wire.beginTransmission(I2C_ADDR_LCD);
    bool ok = (Wire.endTransmission() == 0);
    tca_disable_all();
    return ok;
}

inline bool lcd_keluar_check() {
    tca_select(TCA_CH_LCD_KELUAR);
    Wire.beginTransmission(I2C_ADDR_LCD);
    bool ok = (Wire.endTransmission() == 0);
    tca_disable_all();
    return ok;
}

// ── Helper Cetak Baris ───────────────────────────────────────
inline void lcd_masuk_print(uint8_t row, const char* text) {
    tca_select(TCA_CH_LCD_MASUK);
    lcdMasuk.setCursor(0, row);
    lcdMasuk.print("                    "); // bersihkan baris (20 spasi)
    lcdMasuk.setCursor(0, row);
    lcdMasuk.print(text);
    tca_disable_all();
}

inline void lcd_masuk_clear() {
    tca_select(TCA_CH_LCD_MASUK);
    lcdMasuk.clear();
    tca_disable_all();
}

inline void lcd_keluar_print(uint8_t row, const char* text) {
    tca_select(TCA_CH_LCD_KELUAR);
    lcdKeluar.setCursor(0, row);
    lcdKeluar.print("                    ");
    lcdKeluar.setCursor(0, row);
    lcdKeluar.print(text);
    tca_disable_all();
}

inline void lcd_keluar_clear() {
    tca_select(TCA_CH_LCD_KELUAR);
    lcdKeluar.clear();
    tca_disable_all();
}


// ── Layar Idle ───────────────────────────────────────────────
inline void lcd_masuk_idle() {
    lcd_masuk_clear();
    lcd_masuk_print(0, "  --- PARKEASY ---  ");
    lcd_masuk_print(1, "   Gerbang Masuk    ");
    lcd_masuk_print(2, " Dekatkan Kendaraan ");
    lcd_masuk_print(3, " Ke Garis Deteksi..");
}

inline void lcd_keluar_idle() {
    lcd_keluar_clear();
    lcd_keluar_print(0, "  --- PARKEASY ---  ");
    lcd_keluar_print(1, "   Gerbang Keluar   ");
    lcd_keluar_print(2, " Dekatkan Kendaraan ");
    lcd_keluar_print(3, " Ke Garis Deteksi..");
}


// ── Animasi Memindai ─────────────────────────────────────────
inline void lcd_anim_kamera(int isMasuk) {
    if (isMasuk) {
        lcd_masuk_clear();
        lcd_masuk_print(0, "== GERBANG MASUK == ");
        lcd_masuk_print(1, "Memindai Kendaraan..");
        lcd_masuk_print(2, "  [ KAMERA AKTIF ]  ");
        lcd_masuk_print(3, "[====>         ] 30%");
    } else {
        lcd_keluar_clear();
        lcd_keluar_print(0, "== GERBANG KELUAR ==");
        lcd_keluar_print(1, "Memindai Kendaraan..");
        lcd_keluar_print(2, "  [ KAMERA AKTIF ]  ");
        lcd_keluar_print(3, "[====>         ] 30%");
    }
}

inline void lcd_anim_database(int isMasuk) {
    if (isMasuk) {
        lcd_masuk_clear();
        lcd_masuk_print(0, "== GERBANG MASUK == ");
        lcd_masuk_print(1, "Plat Terdeteksi!    ");
        lcd_masuk_print(2, "Mencocokkan Data... ");
        lcd_masuk_print(3, "[=========>    ] 70%");
    } else {
        lcd_keluar_clear();
        lcd_keluar_print(0, "== GERBANG KELUAR ==");
        lcd_keluar_print(1, "Plat Terdeteksi!    ");
        lcd_keluar_print(2, "Mencocokkan Data... ");
        lcd_keluar_print(3, "[=========>    ] 70%");
    }
}

inline void lcd_anim_sukses_100(int isMasuk) {
    if (isMasuk) {
        lcd_masuk_clear();
        lcd_masuk_print(0, "== GERBANG MASUK == ");
        lcd_masuk_print(1, "Data Valid!         ");
        lcd_masuk_print(2, "Membuka Palang...   ");
        lcd_masuk_print(3, "[==============]100%");
    } else {
        lcd_keluar_clear();
        lcd_keluar_print(0, "== GERBANG KELUAR ==");
        lcd_keluar_print(1, "Data Valid!         ");
        lcd_keluar_print(2, "Membuka Palang...   ");
        lcd_keluar_print(3, "[==============]100%");
    }
}


// ── Pesan Hasil Masuk ────────────────────────────────────────
inline void lcd_masuk_sukses(const char* plate, const char* timeStr) {
    lcd_masuk_clear();
    lcd_masuk_print(0, "== GERBANG MASUK == ");
    lcd_masuk_print(1, timeStr);
    lcd_masuk_print(2, "Plat Nomor:");
    lcd_masuk_print(3, plate);
}

inline void lcd_masuk_darurat(const char* plate, const char* timeStr) {
    lcd_masuk_clear();
    lcd_masuk_print(0, "== AKSES DARURAT ==");
    lcd_masuk_print(1, " Masuk Manual Aktif ");
    lcd_masuk_print(2, plate);
    lcd_masuk_print(3, timeStr);
}

inline void lcd_masuk_gagal(const char* plate, const char* reason) {
    lcd_masuk_clear();
    lcd_masuk_print(0, "== GERBANG MASUK == ");
    lcd_masuk_print(1, "Akses Ditolak!      ");
    lcd_masuk_print(2, reason);
    lcd_masuk_print(3, plate);
}

inline void lcd_masuk_tidak_terdeteksi() {
    lcd_masuk_clear();
    lcd_masuk_print(0, "== GERBANG MASUK == ");
    lcd_masuk_print(1, "Tidak Terdeteksi    ");
    lcd_masuk_print(2, "Plat Nomor          ");
    lcd_masuk_print(3, "Coba Mundur/Maju    ");
}


// ── Pesan Hasil Keluar ───────────────────────────────────────
inline void lcd_keluar_sukses(const char* plate) {
    lcd_keluar_clear();
    lcd_keluar_print(0, "== GERBANG KELUAR ==");
    lcd_keluar_print(1, "Plat Nomor:");
    lcd_keluar_print(2, plate);
    lcd_keluar_print(3, "Selamat Jalan!");
}

inline void lcd_keluar_darurat(const char* plate) {
    lcd_keluar_clear();
    lcd_keluar_print(0, "== AKSES DARURAT ==");
    lcd_keluar_print(1, "Keluar Manual Aktif ");
    lcd_keluar_print(2, "Plat Nomor:");
    lcd_keluar_print(3, plate);
}

inline void lcd_keluar_belum_bayar(const char* plate, const char* priceStr) {
    lcd_keluar_clear();
    lcd_keluar_print(0, "== GERBANG KELUAR ==");
    lcd_keluar_print(1, "Harap Membayar Dulu!");
    
    char tagihan[21];
    snprintf(tagihan, sizeof(tagihan), "Rp %s", priceStr);
    lcd_keluar_print(2, tagihan);
    lcd_keluar_print(3, plate);
}

inline void lcd_keluar_gagal(const char* plate, const char* reason) {
    lcd_keluar_clear();
    lcd_keluar_print(0, "== GERBANG KELUAR ==");
    lcd_keluar_print(1, "Akses Ditolak!      ");
    lcd_keluar_print(2, reason);
    lcd_keluar_print(3, plate);
}

inline void lcd_reset_system() {
    lcd_masuk_clear();
    lcd_masuk_print(0, "== RESET SISTEM == ");
    lcd_masuk_print(1, "ESP32 Sedang Reset  ");
    lcd_masuk_print(2, "Tunggu Inisialisasi ");
    lcd_masuk_print(3, "Ulang Perangkat...  ");

    lcd_keluar_clear();
    lcd_keluar_print(0, "== RESET SISTEM == ");
    lcd_keluar_print(1, "ESP32 Sedang Reset  ");
    lcd_keluar_print(2, "Tunggu Inisialisasi ");
    lcd_keluar_print(3, "Ulang Perangkat...  ");
}

inline void lcd_keluar_tidak_terdeteksi() {
    lcd_keluar_clear();
    lcd_keluar_print(0, "== GERBANG KELUAR ==");
    lcd_keluar_print(1, "Tidak Terdeteksi    ");
    lcd_keluar_print(2, "Plat Nomor          ");
    lcd_keluar_print(3, "Coba Mundur/Maju    ");
}

#endif // LCD_DISPLAY_H

