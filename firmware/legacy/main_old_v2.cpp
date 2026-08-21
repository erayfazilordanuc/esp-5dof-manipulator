// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_PWMServoDriver.h>

// // I2C Pinleri (ESP32 için varsayılan: SDA=21, SCL=22)
// // Eğer farklı pinler kullanıyorsan Wire.begin(SDA, SCL); şeklinde belirtmelisin.
// Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// #define SERVO_MIN_PULSE  150  
// #define SERVO_MAX_PULSE  600  
// #define SERVO_FREQ       50   
// #define POT_PIN          34    // ESP32'de Analog pin (GPIO 34)

// struct ServoMotor {
//   int channel;
//   float currentAngle;
//   float targetAngle;
//   float stepSize;
// };

// ServoMotor servos[4] = {
//   {0, 90.0, 90.0, 1.2}, 
//   {1, 90.0, 90.0, 1.2},
//   {2, 90.0, 90.0, 1.2},
//   {3, 90.0, 90.0, 1.2}
// };

// unsigned long lastUpdate = 0;
// const int updateInterval = 10; 

// int angleToPulse(float angle) {
//   return (int)(SERVO_MIN_PULSE + ((angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE)) / 180.0));
// }

// void setup() {
//   Serial.begin(115200);
  
//   // ESP32 I2C başlatma (SDA=21, SCL=22 varsayılan)
//   Wire.begin(21, 22); 

//   pwm.begin();
//   pwm.setOscillatorFrequency(27000000);
//   pwm.setPWMFreq(SERVO_FREQ);

//   for (int i = 0; i < 4; i++) {
//     pwm.setPWM(servos[i].channel, 0, angleToPulse(servos[i].currentAngle));
//   }
  
//   Serial.println("ESP32 Pot Kontrolu Hazir!");
// }

// void loop() {
//   // ESP32 12-bit okuma yaptığı için 4095'e kadar değer gelir
//   int potValue = analogRead(POT_PIN);
//   float newTarget = map(potValue, 0, 4095, 0, 180);

//   for (int i = 0; i < 4; i++) {
//     servos[i].targetAngle = newTarget;
//   }

//   if (millis() - lastUpdate >= updateInterval) {
//     lastUpdate = millis();

//     for (int i = 0; i < 4; i++) {
//       if (abs(servos[i].currentAngle - servos[i].targetAngle) > 0.1) {
//         if (servos[i].currentAngle < servos[i].targetAngle) {
//           servos[i].currentAngle += servos[i].stepSize;
//         } else {
//           servos[i].currentAngle -= servos[i].stepSize;
//         }
//         pwm.setPWM(servos[i].channel, 0, angleToPulse(servos[i].currentAngle));
//       }
//     }
//   }
// }