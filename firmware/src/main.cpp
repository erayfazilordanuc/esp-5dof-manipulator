#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "secrets.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

#define NUM_SERVOS 5
#define PCA9685_ADDR 0x40
#define SERVO_FREQ 50
#define OSCILLATOR_FREQ 25000000

#define USMIN 500
#define USMAX 2500

struct SpeedProfile
{
    double step;
    int delay;
};

#define SLOW_STEP 0.4
#define SLOW_DELAY 12

SpeedProfile normalSpeeds[NUM_SERVOS] = {
    {1.5, 8},
    {0.8, 10},
    {2.0, 6},
    {3.0, 5},
    {4.0, 4}};

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

class SmartServo
{
private:
    uint8_t _channel;
    double _currentAngle;
    int _targetAngle;
    unsigned long _lastUpdate;
    double _stepSize;
    int _delayTime;
    int _lastPulseWritten;

public:
    bool isAtTarget;

    SmartServo() {}

    void attach(uint8_t channel)
    {
        _channel = channel;
        _currentAngle = 0.0;
        _targetAngle = 90;
        _lastUpdate = 0;
        _lastPulseWritten = -1;
        isAtTarget = false;

        setSpeed(SLOW_STEP, SLOW_DELAY);
    }

    void setSpeed(double step, int delay)
    {
        _stepSize = step;
        _delayTime = delay;
    }

    void setTarget(int angle)
    {
        if (angle < 0)
            angle = 0;
        if (angle > 180)
            angle = 180;
        _targetAngle = angle;
        isAtTarget = false;
    }

    void update()
    {
        if (millis() - _lastUpdate > _delayTime)
        {
            _lastUpdate = millis();
            double error = _targetAngle - _currentAngle;

            if (abs(error) >= _stepSize)
            {
                if (error > 0)
                    _currentAngle += _stepSize;
                else
                    _currentAngle -= _stepSize;

                writeToMotor();
                isAtTarget = false;
            }
            else
            {
                if (!isAtTarget)
                {
                    _currentAngle = _targetAngle;
                    writeToMotor();
                    isAtTarget = true;
                }
            }
        }
    }

private:
    void writeToMotor()
    {
        double pulse = USMIN + (_currentAngle / 180.0) * (USMAX - USMIN);
        int intPulse = (int)pulse;
        if (intPulse != _lastPulseWritten)
        {
            pwm.writeMicroseconds(_channel, intPulse);
            _lastPulseWritten = intPulse;
        }
    }
};

SmartServo robotArm[NUM_SERVOS];
bool systemInitialized = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>YTÜ Robot Arm</title>
  <style>
    :root { --primary: #00e5ff; --bg: #121212; --card: #1e1e1e; --text: #e0e0e0; }
    body { background-color: var(--bg); color: var(--text); font-family: sans-serif; text-align: center; margin: 0; padding: 20px; }
    h1 { color: var(--primary); margin-bottom: 20px; text-transform: uppercase; }
    .control-panel { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; max-width: 800px; margin: 0 auto; }
    .card { background-color: var(--card); border-radius: 12px; padding: 20px; border: 1px solid #333; }
    label { display: block; margin-bottom: 10px; font-weight: bold; }
    input[type=range] { width: 100%; -webkit-appearance: none; background: #333; height: 6px; border-radius: 3px; outline: none; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%; background: var(--primary); cursor: pointer; }
    .status { display: flex; justify-content: space-between; margin-top: 5px; font-size: 0.9rem; color: #888; }
    .angle-val { color: var(--primary); font-weight: bold; }
  </style>
</head>
<body>
  <h1>Mekatronik Robot Kontrol</h1>
  <div class="control-panel" id="panel"></div>
<script>
  const motorNames = ["Taban (Base)", "Omuz (Shoulder)", "Dirsek (Elbow)", "Bilek (Pitch)", "Kıskaç (Gripper)"];
  const gateway = `ws:
  let websocket;

  function initUI() {
    const container = document.getElementById('panel');
    motorNames.forEach((name, index) => {
        let html = `
        <div class="card">
            <label>${name}</label>
            <input type="range" min="0" max="180" value="90" id="s${index}" oninput="sendData(${index}, this.value)">
            <div class="status">
                <span>ID: ${index}</span>
                <span class="angle-val"><span id="v${index}">90</span>°</span>
            </div>
        </div>`;
        container.innerHTML += html;
    });
  }

  function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onopen = (e) => console.log('Bağlı');
    websocket.onclose = (e) => setTimeout(initWebSocket, 2000);
  }

  function sendData(id, val) {
    document.getElementById('v' + id).innerText = val;
    websocket.send(id + ":" + val);
  }

  window.onload = () => { initUI(); initWebSocket(); };
</script>
</body>
</html>
)rawliteral";

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
        {
            data[len] = 0;
            String message = (char *)data;
            int separatorIndex = message.indexOf(':');
            if (separatorIndex != -1)
            {
                int id = message.substring(0, separatorIndex).toInt();
                int angle = message.substring(separatorIndex + 1).toInt();
                if (id >= 0 && id < NUM_SERVOS)
                {
                    robotArm[id].setTarget(angle);
                }
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Wire.begin();
    pwm.begin();
    pwm.setOscillatorFrequency(OSCILLATOR_FREQ);
    pwm.setPWMFreq(SERVO_FREQ);

    for (int i = 0; i < NUM_SERVOS; i++)
    {
        robotArm[i].attach(i);
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);
    Serial.println("\nIP: " + WiFi.localIP().toString());

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html); });
    server.begin();
}

void loop()
{
    ws.cleanupClients();

    bool allAtTarget = true;
    for (int i = 0; i < NUM_SERVOS; i++)
    {
        robotArm[i].update();
        if (!robotArm[i].isAtTarget)
            allAtTarget = false;
    }

    if (!systemInitialized && allAtTarget)
    {
        Serial.println(">>> Sistem Hazır: Bireysel Hız Profilleri Yükleniyor <<<");

        for (int i = 0; i < NUM_SERVOS; i++)
        {
            robotArm[i].setSpeed(normalSpeeds[i].step, normalSpeeds[i].delay);
        }

        systemInitialized = true;
    }
}