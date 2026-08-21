/**
 * =============================================================================
 *  ArmPilot - 5 eksen robot kol kontrolcusu
 *  Yazar : Eray Fazil Ordanuc
 *  Donanim: ESP32 + PCA9685 + 5x analog servo
 * =============================================================================
 *
 *  MIMARI
 *  ------
 *  Core 1 / oncelik 3 : "motion" gorevi -> 100 Hz sabit periyot, hareket
 *                       profillerini isler ve PCA9685'e yazar. WiFi trafigi
 *                       hareketin duzgunlugunu etkilemez.
 *  Core * / oncelik 1 : Arduino loop() -> telemetri yayini, baglanti bakimi.
 *  AsyncTCP gorevi    : WebSocket komutlari -> ArmController'a guvenli aktarim.
 *
 *  GUVENLI ACILIS
 *  --------------
 *  Acilista hicbir servo kanalina darbe gonderilmez (PCA9685 "full-off").
 *  Servolar torksuz kalir, yani kol acilista ASLA sicramaz. Kullanici arayuzden
 *  "ETKINLESTIR" dedigi anda, NVS'te saklanan son poz kanal kanal (araya
 *  gecikme koyarak) uygulanir. Yazilan aci fiziksel konumla ayni oldugu icin
 *  servo yerinden oynamaz; ayrica kanallar sirayla acildigi icin akim piki
 *  olusmaz ve besleme cokmez.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_system.h>

#include "config.h"
#include "arm_controller.h"
#include "web_ui.h"

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

// ===========================================================================
//  Reset teshisi
//  "Servolar durduk yere serbest kaliyor" sikayetinin en sik sebebi ESP32'nin
//  resetlenmesidir (servolar akim cekince besleme cokerse BROWNOUT). Reset
//  olunca PCA9685 cikislari kapanir ve kol torksuz kalir. Sebebi burada
//  yakalayip hem seri porta hem arayuze bildiriyoruz.
// ===========================================================================
static const char *resetReasonText()
{
    switch (esp_reset_reason())
    {
    case ESP_RST_POWERON:  return "GUC VERILDI";
    case ESP_RST_EXT:      return "HARICI RESET";
    case ESP_RST_SW:       return "YAZILIM RESET";
    case ESP_RST_PANIC:    return "PANIC (yazilim hatasi)";
    case ESP_RST_INT_WDT:  return "WATCHDOG (int)";
    case ESP_RST_TASK_WDT: return "WATCHDOG (gorev)";
    case ESP_RST_WDT:      return "WATCHDOG";
    case ESP_RST_BROWNOUT: return "BROWNOUT (besleme cokmesi!)";
    case ESP_RST_DEEPSLEEP:return "DERIN UYKU";
    case ESP_RST_SDIO:     return "SDIO";
    default:               return "BILINMIYOR";
    }
}
static const char *g_resetReason = "?";

// ===========================================================================
//  Telemetri
// ===========================================================================
static String buildHello()
{
    float cur[NUM_JOINTS], tg[NUM_JOINTS];
    arm.snapshot(cur, tg);

    String s;
    s.reserve(900);
    s += F("{\"t\":\"hello\",\"cur\":[");
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        if (i)
            s += ',';
        s += String(cur[i], 1);
    }
    s += F("],\"cfg\":{\"fw\":\"");
    s += F(FW_VERSION);
    s += F("\",\"host\":\"");
    s += F(MDNS_HOST ".local");
    s += F("\",\"ip\":\"");
    s += (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    s += F("\",\"rst\":\"");
    s += g_resetReason;

    s += F("\",\"geo\":{\"baseR\":");
    s += String(GEOMETRY.baseRadius, 1);
    s += F(",\"baseH\":");
    s += String(GEOMETRY.baseHeight, 1);
    s += F(",\"l1\":");
    s += String(GEOMETRY.linkUpperArm, 1);
    s += F(",\"l2\":");
    s += String(GEOMETRY.linkForearm, 1);
    s += F(",\"l3\":");
    s += String(GEOMETRY.linkGripper, 1);

    s += F("},\"j\":[");
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        if (i)
            s += ',';
        s += F("{\"n\":\"");
        s += JOINTS[i].name;
        s += F("\",\"lo\":");
        s += String(JOINTS[i].minDeg, 0);
        s += F(",\"hi\":");
        s += String(JOINTS[i].maxDeg, 0);
        s += '}';
    }
    s += F("]}}");
    return s;
}

/** Kalibrasyon paketi - baglantida ve her degisiklikte gonderilir. */
static String buildCal()
{
    const CalData &c = arm.cal();
    String s;
    s.reserve(260);
    s += F("{\"t\":\"cal\",\"sref\":[");
    s += String(c.sRef[0], 1) + ',' + String(c.sRef[1], 1) + ',' + String(c.sRef[2], 1);
    s += F("],\"wref\":[");
    s += String(c.wRef[0], 1) + ',' + String(c.wRef[1], 1) + ',' + String(c.wRef[2], 1);
    s += F("],\"gain\":[");
    s += String(c.gain[0], 4) + ',' + String(c.gain[1], 4) + ',' + String(c.gain[2], 4);
    s += F("],\"gr\":[");
    s += String(c.gripOpen, 1) + ',' + String(c.gripClose, 1);
    s += F("],\"par\":");
    s += String((int)c.parallel);
    s += F(",\"smp\":[");
    s += String(arm.calSampleValid(0) ? 1 : 0) + ',' + String(arm.calSampleValid(1) ? 1 : 0);
    // Kaydedilmis orneklerin servo acilari: arayuz "bu eksen yeterince oynadi
    // mi" geri bildirimini bunlardan uretir.
    for (uint8_t k = 0; k < 2; k++)
    {
        const CalSample &sm = arm.calSampleData(k);
        s += (k == 0) ? F("],\"s0\":[") : F("],\"s1\":[");
        s += String(sm.servo[0], 1) + ',' + String(sm.servo[1], 1) + ',' + String(sm.servo[2], 1);
    }
    s += F("],\"fit\":");
    s += String((int)arm.lastFitMask());
    s += '}';
    return s;
}

static size_t buildState(char *buf, size_t cap)
{
    float cur[NUM_JOINTS], tg[NUM_JOINTS];
    arm.snapshot(cur, tg);

    return snprintf(buf, cap,
                    "{\"t\":\"s\",\"s\":%u,\"m\":%u,\"k\":%u,\"v\":%.2f,\"up\":%lu,"
                    "\"c\":[%.1f,%.1f,%.1f,%.1f,%.1f],"
                    "\"g\":[%.1f,%.1f,%.1f,%.1f,%.1f]}",
                    (unsigned)arm.state(), arm.isMoving() ? 1u : 0u, arm.poseKnown() ? 1u : 0u,
                    arm.speedScale(), (unsigned long)(millis() / 1000UL),
                    cur[0], cur[1], cur[2], cur[3], cur[4],
                    tg[0], tg[1], tg[2], tg[3], tg[4]);
}

// ===========================================================================
//  Komut ayristirma
//    "a <a0> <a1> <a2> <a3> <a4>"  tum eksenler
//    "j <id> <aci>"                tek eksen
//    "s <olcek>"                   hiz olcegi (0.15 .. 1.30)
//    "c <komut>"                   engage|release|home|park|stop|resume|save
//    "k <alt-komut>"               kalibrasyon (bkz. asagi)
// ===========================================================================
static volatile bool g_calChanged = false;

static void handleCommand(char *msg)
{
    char *save = nullptr;
    const char *op = strtok_r(msg, " \t", &save);
    if (!op)
        return;

    switch (op[0])
    {
    case 'a':
    {
        float a[NUM_JOINTS];
        for (uint8_t i = 0; i < NUM_JOINTS; i++)
        {
            const char *tok = strtok_r(nullptr, " \t", &save);
            if (!tok)
                return; // eksik paket -> yok say
            a[i] = strtof(tok, nullptr);
        }
        arm.setAllTargets(a);
        break;
    }
    case 'j':
    {
        const char *sid = strtok_r(nullptr, " \t", &save);
        const char *sv = strtok_r(nullptr, " \t", &save);
        if (sid && sv)
            arm.setJointTarget((uint8_t)atoi(sid), strtof(sv, nullptr));
        break;
    }
    case 's':
    {
        const char *sv = strtok_r(nullptr, " \t", &save);
        if (sv)
            arm.setSpeedScale(strtof(sv, nullptr));
        break;
    }
    case 'c':
    {
        const char *c = strtok_r(nullptr, " \t\r\n", &save);
        if (!c)
            return;
        if (!strcmp(c, "engage"))
            arm.requestEngage();
        else if (!strcmp(c, "release"))
            arm.requestRelease();
        else if (!strcmp(c, "home"))
            arm.requestHome();
        else if (!strcmp(c, "park"))
            arm.requestPark();
        else if (!strcmp(c, "stop"))
            arm.requestStop();
        else if (!strcmp(c, "resume"))
            arm.requestResume();
        else if (!strcmp(c, "save"))
            arm.requestSave();
        break;
    }
    case 'k':
    {
        // k c                      -> mevcut pozu "dimdik yukari" referansi al
        // k d <0..2>               -> omuz/dirsek/bilek yonunu cevir
        // k m <0|1>                -> kinematik modu (0 seri, 1 paralel)
        // k g o|c                  -> kiskac tam acik / tam kapali referansi
        // k r                      -> fabrika ayarlarina don
        // k p <0|1> <q0> <q1> <q2> -> kalibrasyon modu: esleme ornegi kaydet
        // k f                      -> iki ornekten gain/referans coz ve kaydet
        // k x                      -> ornekleri sil
        const char *sub = strtok_r(nullptr, " \t\r\n", &save);
        if (!sub)
            return;
        const char *argp = strtok_r(nullptr, " \t\r\n", &save);

        switch (sub[0])
        {
        case 'c':
            arm.calCapture();
            break;
        case 'd':
            if (!argp)
                return;
            arm.calFlipDir((uint8_t)atoi(argp));
            break;
        case 'm':
            if (!argp)
                return;
            arm.calSetParallel(atoi(argp) != 0);
            break;
        case 'g':
            if (!argp)
                return;
            arm.calSetGripper(argp[0] == 'o');
            break;
        case 'r':
            arm.calReset();
            arm.calSave();
            break;
        case 'p':
        {
            if (!argp)
                return;
            float q[3];
            for (uint8_t i = 0; i < 3; i++)
            {
                const char *tok = strtok_r(nullptr, " \t\r\n", &save);
                if (!tok)
                    return; // eksik paket -> yok say
                q[i] = strtof(tok, nullptr);
            }
            arm.calSample((uint8_t)atoi(argp), q);
            break;
        }
        case 'f':
            arm.calFit();
            break;
        case 'x':
            arm.calClearSamples();
            break;
        default:
            return;
        }

        g_calChanged = true;
        break;
    }
    default:
        break;
    }
}

static void onWsEvent(AsyncWebSocket *, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("[WS] istemci #%u baglandi\n", client->id());
        client->text(buildHello());
        client->text(buildCal());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("[WS] istemci #%u ayrildi\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT))
            return;
        if (len == 0 || len > 127)
            return;

        char buf[128];
        memcpy(buf, data, len);
        buf[len] = '\0';
        handleCommand(buf);
    }
}

// ===========================================================================
//  Hareket gorevi - sabit periyot, WiFi'den bagimsiz
// ===========================================================================
static void motionTask(void *)
{
    const TickType_t period = pdMS_TO_TICKS(1000 / CTRL_HZ);
    const float dt = 1.0f / (float)CTRL_HZ;
    TickType_t last = xTaskGetTickCount();

    for (;;)
    {
        arm.tick(dt);
        vTaskDelayUntil(&last, period);
    }
}

// ===========================================================================
//  Ag
// ===========================================================================
static void startNetwork()
{
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // dusuk gecikmeli WebSocket icin
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[NET] '%s' agina baglaniliyor", WIFI_SSID);

    const uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS)
    {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("[NET] Baglandi. IP: %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        // Ag yoksa kendi erisim noktasini ac; robot her kosulda kontrol edilebilsin.
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        Serial.printf("[NET] Ag bulunamadi. AP acildi: %s / %s  IP: %s\n",
                      AP_SSID, AP_PASSWORD, WiFi.softAPIP().toString().c_str());
    }

    if (MDNS.begin(MDNS_HOST))
    {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[NET] http://%s.local\n", MDNS_HOST);
    }
}

// ===========================================================================
void setup()
{
    Serial.begin(115200);
    delay(200);
    g_resetReason = resetReasonText();
    Serial.printf("\n\n=== %s v%s ===\n", FW_NAME, FW_VERSION);
    Serial.printf("[SYS] Onceki reset sebebi: %s\n", g_resetReason);
    if (esp_reset_reason() == ESP_RST_BROWNOUT)
        Serial.println("[SYS] !! Besleme cokuyor. Servolara AYRI 5-6V kaynak ver, "
                       "GND'leri birlestir, servo hattina 1000uF kondansator koy.");

    // Once kol: cikislar kapali baslasin, servolar serbest kalsin.
    arm.begin();

    startNetwork();

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
              {
        // Sayfa dogrudan flash'tan akitilir; ~20 KB'lik heap kopyasi olusmaz.
        AsyncWebServerResponse *res = req->beginResponse(
            200, "text/html", (const uint8_t *)INDEX_HTML, strlen_P(INDEX_HTML));
        res->addHeader("Cache-Control", "no-store");
        req->send(res); });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send(204); });

    server.onNotFound([](AsyncWebServerRequest *req)
                      { req->redirect("/"); });

    server.begin();
    Serial.println("[WEB] Sunucu ayakta.");

    // Yigin: profil matematigi + NVS yazimi + Serial.printf icin rahat pay.
    xTaskCreatePinnedToCore(motionTask, "motion", 6144, nullptr, 3, nullptr, 1);
    Serial.println("[SYS] Hareket gorevi baslatildi (core 1, 100 Hz).");
    Serial.println("[SYS] Servolar SERBEST. Arayuzden ETKINLESTIR ile devreye al.");
}

void loop()
{
    static uint32_t lastTelemetry = 0;
    static uint32_t lastCleanup = 0;
    const uint32_t now = millis();

    if (now - lastCleanup >= 1000)
    {
        lastCleanup = now;
        ws.cleanupClients();
    }

    if (g_calChanged)
    {
        g_calChanged = false;
        if (ws.count())
            ws.textAll(buildCal());
    }

    if (now - lastTelemetry >= (1000 / TELEMETRY_HZ))
    {
        lastTelemetry = now;
        if (ws.count() && ws.availableForWriteAll())
        {
            char buf[256];
            const size_t n = buildState(buf, sizeof(buf));
            if (n > 0 && n < sizeof(buf))
                ws.textAll(buf, n);
        }
    }

    delay(2);
}
