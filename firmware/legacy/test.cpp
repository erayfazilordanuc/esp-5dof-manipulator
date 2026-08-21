// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_PWMServoDriver.h>

// Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// #define SERVO_MIN_PULSE  150  
// #define SERVO_MAX_PULSE  600  
// #define SERVO_FREQ       50   

// // Açıdan Pulse değerine çeviren fonksiyon
// int angleToPulse(int angle) {
//   return map(angle, 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
// }

// void setup() {
//   Serial.begin(115200);
  
//   // ESP32 I2C Pinleri: SDA=21, SCL=22
//   Wire.begin(21, 22); 

//   pwm.begin();
//   pwm.setOscillatorFrequency(27000000);
//   pwm.setPWMFreq(SERVO_FREQ);

//   delay(500); // Sistemin oturması için kısa bir bekleme

//   // SADECE 2. KANAL (Kanal Numarası: 1)
//   // Not: PCA9685 üzerindeki 0, 1, 2 diye giden pinlerden 1 numaralı olana takmalısın.
//   pwm.setPWM(1, 0, angleToPulse(90));
  
//   Serial.println("Kanal 1 (2. Servo) 90 dereceye ayarlandi.");
// }

// void loop() {
//   // Test kodu olduğu için loop boş kalsın.
// }