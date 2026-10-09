/*********
  @author Evan Alif Widhyatma
  @version 4.0 Stable
*********/

//================ INCLUDE ================

#include "esp_sleep.h"
#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#ifndef ENABLE_LORA_OTA_TRIGGER
#define ENABLE_LORA_OTA_TRIGGER 0
#endif

//================ WIFI CONFIG ================
const char* WIFI_SSID = "JERUKAGUNG SEISMOLOGI"; // Ganti dengan SSID Anda
const char* WIFI_PASS = "JERIS6467";             // Ganti dengan Password WiFi

// Jeda mendengarkan sinyal OTA dari Gateway (dipersingkat dari 3000ms menjadi 800ms)
const unsigned long OTA_LISTEN_WINDOW_MS = 800;

#if ENABLE_LORA_OTA_TRIGGER
void startOTAMode();
#endif

#ifdef USE_SHT40
#include <Adafruit_SHT4x.h>
#endif

#ifdef USE_SHT31
#include "Adafruit_SHT31.h"
#endif

#ifdef USE_BMP280
#include <Adafruit_BMP280.h>
#endif

#ifdef USE_MS5611
#include "MS5611.h"
#endif

#ifdef USE_BH1750
#include <BH1750.h>
#endif

#ifdef USE_DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#include "Adafruit_MAX1704X.h"

//================ DEVICE CONFIG ================

uint64_t uS_TO_S_FACTOR = 1000000;
uint64_t TIME_TO_SLEEP = 60;

//================ LORA PIN ================

#define ss 5
#define rst 16
#define dio0 4

//================ GPIO ================

const int ledPin = 2;

#ifdef USE_DS18B20
const uint8_t DS18_PIN = 25;
#endif

//================ SENSOR OBJECT ================

#ifdef USE_SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

#ifdef USE_SHT40
Adafruit_SHT4x sht4;
#endif

#ifdef USE_BMP280
Adafruit_BMP280 bmp;
#endif

#ifdef USE_MS5611
MS5611 ms5611(0x77);
#endif

#ifdef USE_BH1750
BH1750 lightMeter;
#endif

#ifdef USE_DS18B20
OneWire oneWire(DS18_PIN);
DallasTemperature dsSensors(&oneWire);
#endif

Adafruit_MAX17048 maxlipo;

//================ PAYLOAD STRUCT ================

#if defined(NODE_AGRO)

struct __attribute__((packed)) NodeAgro {
  uint8_t id;
  int16_t temp;
  uint16_t hum;
  uint32_t press;
  uint16_t volt;
  uint32_t lux;
  int16_t temp_ds;
};

NodeAgro paketAgro;

#elif defined(NODE_METEO)

struct __attribute__((packed)) NodeMeteo {
  uint8_t id;
  int16_t temp;
  uint16_t hum;
  uint32_t press;
  uint16_t volt;
};

NodeMeteo paketMeteo;

#endif

//================ SETUP ================
void setup() {
  delay(1000);
  setCpuFrequencyMhz(80);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Wire.begin();
  Wire.setClock(100000);

//================ SHT31 ================
#ifdef USE_SHT31
  if (!sht31.begin(0x45)) {
    while (1);
  }
  sht31.reset();
#endif

//================ SHT40 ================
#ifdef USE_SHT40
  if (!sht4.begin()) {
    while (1);
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
#endif

//================ BMP280 ================
#ifdef USE_BMP280
  if (!bmp.begin(0x76)) {
    while (1);
  }
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );
#endif

//================ BH1750 ================
#ifdef USE_BH1750
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
  delay(200);
#endif

//================ DS18B20 ================
#ifdef USE_DS18B20
  dsSensors.begin();
#endif

//================ MAX17048 ================
if (!maxlipo.begin()) {
    while (1);
  }
  maxlipo.setAlertVoltages(2.0, 4.2);
//================ READ SENSOR ================
  float tempi = NAN;
  float humi = NAN;
  float press = NAN;
  float volt = NAN;
  float light = 0;
  float soilTemp = NAN;
//================ SHT31 READ ================
#ifdef USE_SHT31
  sht31.readBoth(&tempi, &humi);
  sht31.heater(false);
#endif

//================ SHT40 READ ================
#ifdef USE_SHT40
  sensors_event_t humidityEvent, tempEvent;
  sht4.getEvent(&humidityEvent, &tempEvent);
  tempi = tempEvent.temperature;
  humi = humidityEvent.relative_humidity;
#endif

//================ BMP280 READ ================
#ifdef USE_BMP280
  press = bmp.readPressure() / 100.0F;
#endif

//================ BH1750 READ ================
#ifdef USE_BH1750
  if (lightMeter.measurementReady()) {
    light = lightMeter.readLightLevel();
    if (light < 0 || isnan(light)) {
      light = 0;
    }
  }
#endif

//================ DS18B20 READ ================
#ifdef USE_DS18B20
  dsSensors.requestTemperatures();
  float t = dsSensors.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C) {
    soilTemp = t;
  }
#endif

//================ BATTERY READ ================
volt = maxlipo.cellVoltage();
//================ LOW BATTERY MODE ================
if ((volt * 100) <= 320) {
    uint64_t TIME_TO_SLEEP_LOW_BAT = 600;
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_LOW_BAT * uS_TO_S_FACTOR);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_AUTO);
    esp_deep_sleep_start();
  }

  //================ LORA INIT ================
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(922E6)) {
    delay(500);
  }

  //================ LORA CONFIG ================
  LoRa.setGain(0);
  LoRa.setTxPower(14);
  LoRa.setSpreadingFactor(8);
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(8);
  LoRa.enableCrc();
  LoRa.setSyncWord(0xE3);
  LoRa.setPreambleLength(8);
  //================ BUILD PAYLOAD ================

#if defined(NODE_AGRO)

  paketAgro.id = ID;
  paketAgro.temp = isnan(tempi) ? 0 : tempi * 100;
  paketAgro.hum = isnan(humi) ? 0 : humi * 100;
  paketAgro.press = isnan(press) ? 0 : press * 100;
  paketAgro.volt = isnan(volt) ? 0 : volt * 100;
  paketAgro.lux = isnan(light) ? 0 : light * 100;
  paketAgro.temp_ds = isnan(soilTemp) ? 0 : soilTemp * 100;

#endif
  //================ SEND LORA ================

  LoRa.beginPacket();

#if defined(NODE_AGRO)
  LoRa.write((uint8_t *)&paketAgro,sizeof(paketAgro));
#elif defined(NODE_METEO)
  LoRa.write((uint8_t *)&paketMeteo, sizeof(paketMeteo));
#endif
  LoRa.endPacket();
#ifdef USE_BMP280
  bmp.setSampling(Adafruit_BMP280::MODE_SLEEP);
#endif

  //================ TUNGGU PERINTAH OTA DARI GATEWAY (FAST WINDOW) ================
#if ENABLE_LORA_OTA_TRIGGER
  LoRa.receive();
  unsigned long startListen = millis();
  bool otaRequested = false;

  while (millis() - startListen < OTA_LISTEN_WINDOW_MS) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String incoming = "";
      while (LoRa.available()) {
        incoming += (char)LoRa.read();
      }
      if (incoming == "CMD:OTA") {
        otaRequested = true;
        break;
      }
    }
  }

  if (otaRequested) {
    startOTAMode(); // Masuk ke mode OTA jika diminta gateway
  }
#endif

  LoRa.sleep();
  //================ SLEEP ================
  digitalWrite(ledPin, LOW);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_AUTO);

  esp_deep_sleep_start();
}

#if ENABLE_LORA_OTA_TRIGGER
void startOTAMode() {
  digitalWrite(ledPin, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Koneksi WiFi dengan timeout 15 detik agar baterai tidak terkuras jika WiFi offline
  unsigned long startWifi = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 15000) {
    delay(200);
  }

  // Jika WiFi gagal terhubung, batalkan OTA dan kembali tidur
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    digitalWrite(ledPin, LOW);
    LoRa.sleep();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
    return;
  }

  // Kirim feedback balasan IP ke Gateway via LoRa
  String ipStr = WiFi.localIP().toString();
  String ackStr = "OTA_READY_IP:" + ipStr;
  LoRa.beginPacket();
  LoRa.print(ackStr);
  LoRa.endPacket();

  // Konfigurasi ArduinoOTA
  String hostname = "MeteoSense-Node" + String(ID);
  ArduinoOTA.setHostname(hostname.c_str());

  ArduinoOTA.begin();

  // Loop penanganan OTA dengan Safety Timeout (maks 3 menit)
  // Jika tidak ada proses upload baru dalam 3 menit, node otomatis kembali deep sleep
  unsigned long otaTimeout = millis() + 180000UL;
  while (millis() < otaTimeout) {
    ArduinoOTA.handle();
    delay(10);
  }

  // Timeout selesai tanpa upload, bersihkan dan masuk sleep
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  digitalWrite(ledPin, LOW);
  LoRa.sleep();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}
#endif

void loop() {}