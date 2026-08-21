// /**
//  * Project: ESP32 Robot Arm Controller (PCA9685 + MG90S/SG90)
//  * Author: Eray Fazıl
//  * Description: Smooth servo control implementation with soft-start logic.
//  * Date: 2026-02-10
//  */

// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_PWMServoDriver.h>

// // --- KONFİGÜRASYON SABİTLERİ ---
// #define PCA9685_ADDR    0x40      // Varsayılan I2C adresi
// #define SERVO_FREQ      50        // Analog servolar için standart 50Hz
// #define OSCILLATOR_FREQ 27000000  // PCA9685 dahili osilatör frekansı (Genelde 25MHz veya 27MHz)

// // MG90S ve SG90 için Pulse Genişlikleri (Kalibrasyon gerekebilir)
// // 0 derece = ~500us, 180 derece = ~2400us
// #define USMIN           500       
// #define USMAX           2400      
// #define SERVO_COUNT     4         // Kontrol edilecek servo sayısı

// // --- NESNELER VE DEĞİŞKENLER ---
// Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

// // Servoların anlık konumunu hafızada tutmak için array (State Management)
// int currentAngles[SERVO_COUNT]; 

// // --- FONKSİYON PROTOTİPLERİ ---
// void setServoPulse(uint8_t n, double pulse_us);
// void setServoAngle(uint8_t n, int angle);
// void moveServoSmooth(uint8_t n, int targetAngle, int speedDelay);
// void homeAllServos();

// void setup() {
//   Serial.begin(115200);
//   Serial.println("--- Robot Kol Sistemi Başlatılıyor ---");

//   // I2C Başlatma (ESP32 varsayılan pinleri: SDA=21, SCL=22)
//   Wire.begin();

//   // PCA9685 Başlatma
//   pwm.begin();
//   pwm.setOscillatorFrequency(OSCILLATOR_FREQ);
//   pwm.setPWMFreq(SERVO_FREQ);

//   delay(100); // Sürücünün stabilize olması için kısa bekleme

//   Serial.println("Sistem Hazır. Servolar başlangıç konumuna alınıyor...");
  
//   // İLK AÇILIŞ STRATEJİSİ:
//   // Servoların nerede olduğunu bilmediğimiz için varsayılan olarak 90 derece kabul ediyoruz.
//   // İlk enerjilendirmede servoları 90. dereceye set ediyoruz.
//   for(int i=0; i < SERVO_COUNT; i++) {
//     setServoAngle(i, 90); 
//     currentAngles[i] = 90; // Hafızayı güncelle
//     delay(200); // Ani akım çekimini önlemek için her servo arası bekleme (Staggered Startup)
//   }
  
//   Serial.println("Homing Tamamlandı.");
// }

// void loop() {
//   // TEST SENARYOSU: 4 Servo için Smooth Hareket Testi
  
//   Serial.println("Test: Tüm servolar 45 dereceye yumuşakça gidiyor...");
//   for(int i=0; i < SERVO_COUNT; i++) {
//     moveServoSmooth(i, 45, 10); // 10ms gecikme ile yavaş hareket
//   }
//   delay(1000);

//   Serial.println("Test: Tüm servolar 135 dereceye yumuşakça gidiyor...");
//   for(int i=0; i < SERVO_COUNT; i++) {
//     moveServoSmooth(i, 135, 8); // Biraz daha hızlı
//   }
//   delay(1000);

//   Serial.println("Test: Home konumuna (90) dönüş...");
//   homeAllServos();
//   delay(2000);
// }

// // --- FONKSİYON TANIMLARI ---

// /**
//  * Dereceyi PCA9685 PWM değerine çevirir ve servoya yazar.
//  * @param n: Servo numarası (0-15)
//  * @param angle: Hedef açı (0-180)
//  */
// void setServoAngle(uint8_t n, int angle) {
//   // Güvenlik sınırları
//   if (angle < 0) angle = 0;
//   if (angle > 180) angle = 180;

//   // Arduino map() fonksiyonu long döndürür, hassasiyet için float işlemli map yapıyoruz
//   // 4096 resolution, 1/50Hz = 20ms periyot
//   long pulse_len = map(angle, 0, 180, USMIN, USMAX);
  
//   // Adafruit kütüphanesi writeMicroseconds fonksiyonunu destekler, en temiz yöntemdir.
//   pwm.writeMicroseconds(n, pulse_len);
// }

// /**
//  * Servoyu mevcut konumundan hedef konuma yavaşça götürür.
//  * Blocking (bloklayan) bir fonksiyondur, test için uygundur.
//  * @param n: Servo numarası
//  * @param targetAngle: Hedef açı
//  * @param speedDelay: Her adım arasındaki bekleme süresi (ms). Yüksek değer = Yavaş hareket.
//  */
// void moveServoSmooth(uint8_t n, int targetAngle, int speedDelay) {
//   int startAngle = currentAngles[n];
  
//   if (startAngle < targetAngle) {
//     for (int angle = startAngle; angle <= targetAngle; angle++) {
//       setServoAngle(n, angle);
//       delay(speedDelay);
//     }
//   } else {
//     for (int angle = startAngle; angle >= targetAngle; angle--) {
//       setServoAngle(n, angle);
//       delay(speedDelay);
//     }
//   }
  
//   // Hareketi tamamlayınca hafızayı güncelle
//   currentAngles[n] = targetAngle;
// }

// /**
//  * Tüm servoları sırayla güvenli konuma (90 derece) getirir.
//  */
// void homeAllServos() {
//   for(int i=0; i < SERVO_COUNT; i++) {
//     moveServoSmooth(i, 90, 5); // Orta hızda home
//   }
// }