# LoRa Sender Node

Node pengirim data sensor berbasis **ESP32** untuk sistem **MeteoSense Weather Station**.  
Program ini membaca data dari beberapa sensor lingkungan, membentuk payload biner, lalu mengirimkannya melalui **LoRa**. Setelah proses pengiriman selesai, perangkat masuk ke **deep sleep** untuk menghemat daya.

---

## Fitur Utama

- Membaca suhu dan kelembapan:
  - **SHT31**
  - **SHT40** *(opsional)*
- Membaca tekanan udara:
  - **BMP280**
  - **MS5611** *(opsional)*
- Membaca intensitas cahaya:
  - **BH1750**
- Membaca suhu tanah:
  - **DS18B20**
- Membaca tegangan baterai:
  - **MAX17048 / MAX1704X**
- Mengirim data melalui **LoRa**
- Mode **low battery hibernate**
- Mode hemat daya dengan **deep sleep**

---

## Struktur Proyek

- `src/LoRa-Sender-Node.cpp`  
  File utama program pengirim data.
- `platformio.ini`  
  Konfigurasi environment, build flags, dan library dependency.

---

## Kebutuhan Perangkat

### Mikrokontroler
- ESP32

### Sensor
- SHT31 atau SHT40
- BMP280 atau MS5611
- BH1750
- DS18B20
- MAX17048

### Modul Komunikasi
- LoRa

---

## Library yang Dibutuhkan

Pastikan library berikut tersedia di PlatformIO:

- `LoRa`
- `Adafruit SHT4x Library`
- `Adafruit SHT31 Library`
- `Adafruit BMP280 Library`
- `Adafruit BusIO`
- `Adafruit Unified Sensor`
- `Adafruit MAX1704X`
- `BH1750`
- `OneWire`
- `DallasTemperature`

---

## Konfigurasi PlatformIO

File `platformio.ini` menyediakan dua environment utama:

- `esp32dev_type_meteorologi`
- `esp32dev_type_agrometeorologi`

Masing-masing environment mengaktifkan sensor dan mode node yang berbeda melalui `build_flags`.

### Contoh build flags
```cpp
-D NODE_METEO
-D USE_SHT40
-D USE_BMP280
-D ID=3
```

```cpp
-D NODE_AGRO
-D USE_SHT31
-D USE_BH1750
-D USE_DS18B20
-D ID=1
```

> Pastikan environment yang dipilih sesuai dengan jenis node yang ingin dikompilasi.

---

## Konfigurasi Sensor di Kode

Sensor diaktifkan menggunakan macro di file sumber.

### Sensor yang tersedia
```cpp
#define USE_SHT31
#define USE_SHT40
#define USE_BMP280
#define USE_MS5611
#define USE_BH1750
#define USE_DS18B20
```

### Mode node
Hanya satu mode node yang sebaiknya aktif:

```cpp
#define NODE_AGRO
#define NODE_METEO
```

---

## Pin Mapping

### LoRa
```cpp
#define ss 5
#define rst 16
#define dio0 4
```

### GPIO
```cpp
const int ledPin = 2;
```

### DS18B20
```cpp
const uint8_t DS18_PIN = 25;
```

---

## Alur Kerja Program

1. ESP32 menyala
2. Serial dan sensor diinisialisasi
3. Data sensor dibaca
4. Tegangan baterai diperiksa
5. Jika baterai rendah, perangkat masuk deep sleep lebih lama
6. Modul LoRa diinisialisasi
7. Payload dibentuk
8. Payload dikirim
9. LoRa dimatikan
10. ESP32 masuk deep sleep

---

## Parameter LoRa

Konfigurasi LoRa yang digunakan:

- Frekuensi: **922 MHz**
- Tx Power: **14 dBm**
- Spreading Factor: **8**
- Bandwidth: **250 kHz**
- Coding Rate: **4/8**
- Sync Word: **0xE3**
- Preamble Length: **8**
- CRC: **aktif**

---

## Struktur Payload

### Mode `NODE_AGRO`

```cpp
struct __attribute__((packed)) NodeAgro {
  uint8_t id;
  int16_t temp;
  uint16_t hum;
  uint32_t press;
  uint16_t volt;
  uint32_t lux;
  int16_t temp_ds;
};
```

### Mode `NODE_METEO`

```cpp
struct __attribute__((packed)) NodeMeteo {
  uint8_t id;
  int16_t temp;
  uint16_t hum;
  uint32_t press;
  uint16_t volt;
};
```

---

## Skala Data

Agar payload lebih efisien, nilai dikirim dalam bentuk integer hasil skala:

- `temp` = suhu × 100
- `hum` = kelembapan × 100
- `press` = tekanan × 100
- `volt` = tegangan × 100
- `lux` = cahaya × 100
- `temp_ds` = suhu tanah × 100

### Contoh
- `27.34 °C` → `2734`
- `62.58 %` → `6258`
- `1013.25 hPa` → `101325`

---

## Mode Low Battery

Jika tegangan baterai berada di bawah ambang:

```cpp
if ((volt * 100) <= 320)
```

maka perangkat akan:
- menampilkan pesan `LOW BATTERY HIBERNATE`
- tidur selama `600 detik`
- masuk ke `esp_deep_sleep_start()`

---

## Pengaturan Waktu Tidur

Waktu tidur normal:

```cpp
uint64_t TIME_TO_SLEEP = 60;
```

Artinya node akan tidur selama **60 detik** setelah setiap pengiriman data.

---

## Cara Menggunakan

### 1. Pilih environment PlatformIO
Pilih environment yang sesuai di `platformio.ini`.

### 2. Aktifkan sensor yang dibutuhkan
Sesuaikan macro `USE_...` pada file utama.

### 3. Pilih mode node
Aktifkan hanya salah satu:
- `NODE_AGRO`
- `NODE_METEO`

### 4. Sesuaikan pin bila diperlukan
Pastikan pin LoRa dan DS18B20 sesuai dengan rangkaian.

### 5. Build dan upload
Gunakan PlatformIO untuk compile dan upload ke ESP32.

### 6. Buka Serial Monitor
Gunakan baud rate:

```cpp
115200
```

---

## Output Serial

Saat berjalan, perangkat akan menampilkan pesan seperti:

- `BOOTING NODE...`
- `SHT31 ERROR`
- `BH1750 OK`
- `LORA OK`
- `TX DONE`
- `LOW BATTERY HIBERNATE`

---

## Troubleshooting

### Sensor tidak terdeteksi
- Cek wiring
- Cek alamat I2C
- Nonaktifkan macro sensor yang tidak terpasang

### LoRa gagal inisialisasi
- Pastikan pin LoRa benar
- Cek frekuensi modul
- Cek antena dan catu daya

### Data tidak terbaca di receiver
- Pastikan **frequency**, **sync word**, **SF**, **bandwidth**, dan **coding rate** sama dengan receiver
- Pastikan struktur payload di receiver sama dengan pengirim

### Node cepat masuk sleep
- Cek tegangan baterai
- Periksa log `LOW BATTERY HIBERNATE`

---

## Catatan Penting

- Pastikan hanya satu mode node yang aktif pada satu waktu.
- Pastikan alamat I2C sensor sesuai dengan perangkat keras.
- Jika sensor tidak dipasang, nonaktifkan macro terkait.
- Payload menggunakan format biner, sehingga receiver harus membaca struct dengan urutan dan ukuran field yang sama.
- Jika parameter LoRa tidak sama dengan receiver, komunikasi tidak akan berhasil.

---

## Ringkasan

Repositori ini berisi **node pengirim data sensor** untuk sistem cuaca berbasis **ESP32 + LoRa**.  
Data sensor dibaca secara periodik, dikirim dalam bentuk payload biner, lalu perangkat dihemat dayanya menggunakan **deep sleep**.