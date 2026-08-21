/**
 * ArmPilot - Yapilandirma
 * ---------------------------------------------------------------------------
 * Robotla ilgili TUM ayarlanabilir degerler burada. Arayuz (web) bu dosyadaki
 * geometri/limit bilgilerini baglanti aninda ESP32'den okur; yani mekanik bir
 * degisiklikte SADECE bu dosyayi duzenlemen yeterli.
 */
#pragma once

#include <Arduino.h>

// ============================ KIMLIK =======================================
#define FW_NAME "ArmPilot"
#define FW_VERSION "2.1.0"

// ============================ AG AYARLARI ==================================
// WiFi bilgileri surum kontrolune GIRMEZ. include/secrets_example.h dosyasini
// include/secrets.h olarak kopyalayip kendi aginin bilgilerini gir.
// secrets.h yoksa ornek dosya kullanilir ve derlemede uyari verilir.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets_example.h"
#warning "include/secrets.h bulunamadi -> secrets_example.h kullaniliyor. Kopyalayip WiFi bilgilerini gir."
#endif

#define WIFI_TIMEOUT_MS 15000 // Bu surede baglanamazsa kendi AP'sini acar
#define AP_SSID "ArmPilot-AP"
#define AP_PASSWORD "armpilot1" // En az 8 karakter olmali
#define MDNS_HOST "armpilot"    // http://armpilot.local

// ============================ DONANIM ======================================
#define PCA9685_ADDR 0x40
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_CLOCK_HZ 400000
#define SERVO_FREQ_HZ 50
#define OSCILLATOR_FREQ 25000000UL

// PCA9685 /OE pini. Bagliysa GPIO numarasini yaz (HIGH = tum cikislar kapali).
// Bagli degilse -1 birak; yazilim kanallari "full-off" yaparak ayni isi gorur.
#define PCA9685_OE_PIN -1

// ============================ KONTROL DONGUSU ==============================
#define CTRL_HZ 100             // Hareket profili guncelleme frekansi
#define TELEMETRY_HZ 20         // Arayuze durum yayini
#define ENGAGE_STAGGER_MS 220   // Servolari tek tek devreye alma araligi (akim piki icin)
#define IDLE_SAVE_DELAY_MS 2000 // Hareket bittikten sonra pozu NVS'e yazma gecikmesi
#define MIN_SAVE_INTERVAL_MS 8000

// ============================ EKSENLER =====================================
#define NUM_JOINTS 5

enum JointId : uint8_t
{
    J_BASE = 0,     // Taban donusu (yaw)
    J_SHOULDER = 1, // Omuz
    J_ELBOW = 2,    // Dirsek
    J_WRIST = 3,    // Bilek (pitch)
    J_GRIPPER = 4   // Kiskac
};

/**
 * Tek bir eksenin tum tanimi.
 *  minDeg/maxDeg : yazilimsal guvenlik limitleri (mekanik carpismayi onler)
 *  maxVel        : deg/s   - ust hiz siniri
 *  maxAcc        : deg/s^2 - ivme siniri (yumusak kalkis/durus)
 *  maxJerk       : deg/s^3 - ivme degisim siniri (S-egrisi hissi)
 *  usMin/usMax   : bu servonun 0 ve 180 derecedeki darbe genisligi (kalibrasyon)
 *  invert        : servo ters monteliyse -1 yap (arayuz hep 0..180 mantigi gorur)
 */
struct JointConfig
{
    const char *name;
    uint8_t channel;
    float minDeg;
    float maxDeg;
    float homeDeg;
    float parkDeg;
    float maxVel;
    float maxAcc;
    float maxJerk;
    uint16_t usMin;
    uint16_t usMax;
    int8_t invert;
};

extern const JointConfig JOINTS[NUM_JOINTS];

// ============================ GEOMETRI (mm) ================================
// Arayuzdeki kolun sekli bu olculere gore cizilir. Gercek olculerini gir.
struct ArmGeometry
{
    float baseRadius;   // Taban silindirinin yaricapi (gorsel)
    float baseHeight;   // Zeminden omuz eksenine yukseklik
    float linkUpperArm; // Omuz -> Dirsek
    float linkForearm;  // Dirsek -> Bilek
    float linkGripper;  // Bilek -> Kiskac ucu
};

extern const ArmGeometry GEOMETRY;

// ============================ KALIBRASYON ==================================
// Servo hornlari montajda rastgele bir acida takildigi icin "servo 0 = kol
// duz" gibi bir varsayim yapilamaz. Her eklem icin servo acisi ile eklemin
// gercek acisi arasinda DOGRUSAL bir bagintI kurulur:
//
//   eklem_acisi = wRef + gain * (servo_acisi - sRef)
//
//   sRef : ornek alinan noktadaki servo acisi
//   wRef : ayni noktada eklemin gercek acisi
//   gain : eklem acisi / servo acisi. Isareti YONU (ters monte = negatif),
//          buyuklugu ise varsa disli/kayis ORANINI tasir. Dogrudan tahrikli
//          bir eklemde +1 veya -1 cikar.
//
// "eklem_acisi" ne demek:
//   omuz   -> ust kolun DUNYA acisi (90 = dimdik yukari)
//   dirsek -> seri kinematikte bir onceki uzva GORE aci (0 = uzuvlar ayni
//             dogrultuda), paralel kinematikte dunyada 90'dan sapma
//   bilek  -> ayni mantik
//
// Iki bilinmeyen (sRef/wRef ikilisi ve gain) oldugu icin cozum ICIN IKI ORNEK
// gerekir: arayuzdeki KALIBRASYON MODU'nda kullanici ekrandaki kolu gercek
// kola benzetip iki farkli pozda "nokta kaydet" der, firmware gain'i ve
// referansi bu iki noktadan cozer. Sonuc NVS'e yazilir; asagidakiler sadece
// ilk acilis / fabrika ayari degerleridir.
struct JointCal
{
    float sRef;
    float wRef;
    float gain;
};

extern const JointCal CAL_DEFAULT[3]; // omuz, dirsek, bilek

// Iki ornek arasindaki servo farki bundan kucukse o eksen "oynatilmamis"
// sayilir ve fit edilmez (sifira bolme / gurultuden kaynakli sacma gain).
#define CAL_MIN_SPAN_DEG 8.0f
// Fizik disi gain degerlerini reddet (kablolama/eslesme hatasi yakalar).
#define CAL_GAIN_MIN 0.05f
#define CAL_GAIN_MAX 20.0f

// Paralel (kayis/parallelogram tahrikli) kol ise true. Standart seri kolda
// her servo bir onceki uzva GORE aci uretir -> false.
#define CAL_PARALLEL_DEFAULT false

// Kiskac gorsellestirmesi: servo acisi -> acilma miktari
#define GRIPPER_OPEN_DEG 20.0f  // Tamamen acik kabul edilen servo acisi
#define GRIPPER_CLOSE_DEG 95.0f // Tamamen kapali kabul edilen servo acisi

// ============================ HIZ OLCEGI ===================================
#define SPEED_SCALE_MIN 0.15f
#define SPEED_SCALE_MAX 1.30f
#define SPEED_SCALE_DEFAULT 0.85f
#define HOMING_SPEED_SCALE 0.35f // "Home"/"Park" komutlari bu yavas olcekle gider

// ============================ ACILIS DAVRANISI =============================
// false (ONERILEN): acilista servolar SERBEST kalir, hicbir darbe uretilmez.
//                   Kullanici arayuzden "ETKINLESTIR" deyince, kayitli poz
//                   kanal kanal (staggered) uygulanir -> sicrama olmaz.
// true            : acilista otomatik olarak kayitli pozda kilitlenir ve
//                   ardindan yavasca HOME pozuna gider.
#define AUTO_ENGAGE_ON_BOOT false
#define AUTO_HOME_AFTER_ENGAGE false
