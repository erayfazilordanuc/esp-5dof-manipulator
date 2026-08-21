#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// --- AYARLAR ---
#define SERVO_MIN_PULSE  150  // 0 derece
#define SERVO_MAX_PULSE  600  // 180 derece
#define SERVO_FREQ       50   // 50Hz

// --- SERVO YAPISI (PROFESYONEL YAKLAŞIM) ---
// Her servo için verileri tek tek değişkenlerde tutmak yerine bir 'Yapı' (Struct) oluşturuyoruz.
// Bu sayede kodu kopyala-yapıştır yapmadan yönetebiliriz.
struct ServoMotor {
  int channel;        // PCA üzerindeki port numarası (0, 1, 2, 3)
  float currentAngle; // Şu anki açısı
  float targetAngle;  // Gitmesi gereken hedef açı
  float stepSize;     // Hız adımı (Ne kadar büyükse o kadar hızlı)
};

// 4 Adet Servo Tanımlıyoruz (Kanal, Başlangıç Açısı, Hedef, Hız)
ServoMotor servos[4] = {
  {0, 90.0, 90.0, 0.8},  // 0. Kanal: Biraz yavaş (SG90 olabilir)
  {1, 90.0, 90.0, 1.5},  // 1. Kanal: Hızlı (MG90S olabilir)
  {2, 90.0, 90.0, 0.5},  // 2. Kanal: Çok yavaş ve sinematik
  {3, 90.0, 90.0, 2.0}   // 3. Kanal: Çok hızlı tepki veren
};

// Güncelleme hızı (Döngü hızı)
unsigned long lastUpdate = 0;
const int updateInterval = 10; // 10ms'de bir güncelle (Akıcılık ayarı)

// --- YARDIMCI FONKSİYON ---
int angleToPulse(float angle) {
  return (int)(SERVO_MIN_PULSE + ((angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE)) / 180.0));
}

void setup() {
  Serial.begin(115200);
  
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);

  // Başlangıçta tüm servoları tanımlı oldukları 'currentAngle' konumuna alalım
  for (int i = 0; i < 4; i++) {
    pwm.setPWM(servos[i].channel, 0, angleToPulse(servos[i].currentAngle));
  }
  
  Serial.println("4 Kanal Servo Kontrolu Basladi...");
  delay(500);
}

void loop() {
  // --- SENARYO: HEDEFLERİ BELİRLEME ---
  // Burada servoları hareket ettirmiyoruz, sadece "Oraya git" emri veriyoruz.
  
  static unsigned long scenarioTimer = 0;
  static int phase = 0;

  if (millis() - scenarioTimer > 3000) { // Her 3 saniyede bir yeni pozisyonlar
    scenarioTimer = millis();
    
    Serial.println("Yeni hedefler ataniyor...");
    
    if (phase == 0) {
      // Hepsi sağa baksın
      servos[0].targetAngle = 180;
      servos[1].targetAngle = 180;
      servos[2].targetAngle = 180;
      servos[3].targetAngle = 180;
      phase = 1;
    } else if (phase == 1) {
      // Hepsi sola baksın
      servos[0].targetAngle = 0;
      servos[1].targetAngle = 0;
      servos[2].targetAngle = 0;
      servos[3].targetAngle = 0;
      phase = 2;
    } else {
      // Karışık dursa (Çapraz)
      servos[0].targetAngle = 45;
      servos[1].targetAngle = 135;
      servos[2].targetAngle = 90;
      servos[3].targetAngle = 10;
      phase = 0;
    }
  }

  // --- MOTOR SÜRÜCÜ ÇEKİRDEĞİ (ENGINE) ---
  // Bu kısım sürekli çalışır ve tüm servoları yönetir.
  
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    // 4 Servo için döngü
    for (int i = 0; i < 4; i++) {
      
      // Eğer hedefte değilse hareket ettir
      if (servos[i].currentAngle != servos[i].targetAngle) {
        
        // Hedefe yaklaştır (İleri veya Geri)
        if (servos[i].currentAngle < servos[i].targetAngle) {
          servos[i].currentAngle += servos[i].stepSize;
          // Hedefi geçtiyse düzelt
          if (servos[i].currentAngle > servos[i].targetAngle) servos[i].currentAngle = servos[i].targetAngle;
        } 
        else {
          servos[i].currentAngle -= servos[i].stepSize;
          // Hedefi geçtiyse düzelt
          if (servos[i].currentAngle < servos[i].targetAngle) servos[i].currentAngle = servos[i].targetAngle;
        }

        // Fiziksel olarak PWM sinyalini gönder
        pwm.setPWM(servos[i].channel, 0, angleToPulse(servos[i].currentAngle));
      }?-
    }
  }
}