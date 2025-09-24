# IOT---Peminjaman-Laptop
Sistem IoT ini dirancang untuk mengelola peminjaman laptop secara otomatis menggunakan RFID dan ESP32 sebagai pengendali utama. Identifikasi pengguna dilakukan dengan kartu/tag RFID, data peminjaman dan pengembalian tercatat ke backend, dan admin mengontrol serta menerima notifikasi melalui Bot Telegram.

## Fitur utama

* Identifikasi user menggunakan RFID (UID kartu/tanda pengenal).
* Pencatatan peminjaman & pengembalian otomatis.
* Status perangkat (tersedia/dipinjam) tersimpan di backend lokal atau cloud (SQLite / Firebase / server RESTful).
* Notifikasi dan kontrol admin via Telegram Bot: terima/menolak permintaan peminjaman.
* Web/CLI/API sederhana untuk integrasi lebih lanjut.

---

## Komponen perangkat keras

* **ESP32**
* **RC522**
* **LCD 16x2**
* **I2c**
* **Buzzer**
* **Step Down**
* **Adaptor 5v**
* **Kartu RFID**

---

## Komponen perangkat lunak

* **Firmware ESP32**
* **Backend**
* **Telegram Bot**

---

## Arsitektur sistem (ringkas)

```
[RFID Tag] --> [RFID Reader] --> [ESP32] --> Internet --> [Backend/API] <--> [Database]
                                              |
                                              --> [Telegram Bot] (Admin)
```

---

## Alur kerja (use-case)

1. Pengguna menyodorkan kartu RFID ke reader.
2. ESP32 membaca UID dan mengirim request ke backend: *cek status user & laptop tersedia*.
3. Jika memenuhi syarat, backend mencatat peminjaman (user, timestamp,) dan mengembalikan respons sukses.
4. ESP32 menyalakan buzzer.
5. Backend mengirim notifikasi ke admin melalui Telegram Bot.
6. Saat pengembalian, proses membaca ulang kartu akan mencatat pengembalian.

---

## Pengembangan lebih lanjut (ide)

* Integrasi dengan sistem absensi Sekolah.
* Dashboard web real-time (socket.io) untuk monitoring status perangkat.
* Analitik pemakaian & laporan periode.
* Authentication pengguna via OTP/QR sebagai alternatif RFID.

---

## Kontak

Jika ingin kontribusi atau ada pertanyaan, buka issue di GitHub atau kirim PR.

---
