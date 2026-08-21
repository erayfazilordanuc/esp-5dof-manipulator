/**
 * ArmPilot - Kol kontrolcusu uygulamasi
 */
#include "arm_controller.h"
#include <Wire.h>
#include <string.h>

ArmController arm;

static const char *NVS_NAMESPACE = "armpilot";
static const char *NVS_POSE_KEY = "pose";
static const char *NVS_CAL_KEY = "cal";

// ---------------------------------------------------------------------------
//  NVS erisimi (iki gorevden gelebilir -> serilestir)
// ---------------------------------------------------------------------------
bool ArmController::nvsOpen()
{
    if (!_nvsLock || xSemaphoreTake(_nvsLock, pdMS_TO_TICKS(250)) != pdTRUE)
        return false;
    if (!_prefs.begin(NVS_NAMESPACE, false))
    {
        xSemaphoreGive(_nvsLock);
        return false;
    }
    return true;
}

void ArmController::nvsClose()
{
    _prefs.end();
    xSemaphoreGive(_nvsLock);
}

// ---------------------------------------------------------------------------
//  Kurulum
// ---------------------------------------------------------------------------
void ArmController::begin()
{
    _nvsLock = xSemaphoreCreateMutex();

    // 1) /OE pini varsa daha I2C'ye dokunmadan cikislari kapat.
#if PCA9685_OE_PIN >= 0
    pinMode(PCA9685_OE_PIN, OUTPUT);
    digitalWrite(PCA9685_OE_PIN, HIGH); // HIGH = tum cikislar kapali
#endif

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);

    _pwm.begin();
    _pwm.setOscillatorFrequency(OSCILLATOR_FREQ);
    _pwm.setPWMFreq(SERVO_FREQ_HZ);

    // 2) KRITIK: hicbir kanala darbe gonderme. Servolar serbest kalsin.
    //    (Eski kodda acilista dogrudan aci yazildigi icin servolar ani
    //     sicriyordu; sicramanin sebebi buydu.)
    disableOutputs();

    // 3) Son bilinen pozu ve kalibrasyonu geri yukle
    loadPose();
    loadCal();

    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        const JointConfig &j = JOINTS[i];
        _axis[i].begin(_savedPose[i], j.maxVel, j.maxAcc, j.maxJerk, j.minDeg, j.maxDeg);
        _axis[i].setScale(_speedScale);
        _cmdTarget[i] = _axis[i].position();
    }

    _state = ARM_DISARMED;

#if AUTO_ENGAGE_ON_BOOT
    _pendingHomeAfterEngage = AUTO_HOME_AFTER_ENGAGE;
    _reqEngage = true;
#endif

    Serial.printf("[ARM] Hazir. Durum: SERBEST (tork yok). Kayitli poz: %s\n",
                  _poseKnown ? "VAR" : "YOK (varsayilan kullanildi)");
}

// ---------------------------------------------------------------------------
//  Cikislar
// ---------------------------------------------------------------------------
void ArmController::disableOutputs()
{
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        // PCA9685'te OFF register'inin 12. biti (4096) "full off" demektir:
        // kanalda hic darbe uretilmez -> servo torksuz kalir.
        _pwm.setPWM(JOINTS[i].channel, 0, 4096);
        _lastPulse[i] = -1;
    }
#if PCA9685_OE_PIN >= 0
    digitalWrite(PCA9685_OE_PIN, HIGH);
#endif
}

void ArmController::writeChannel(uint8_t idx, float angle)
{
    const JointConfig &j = JOINTS[idx];

    float a = constrain(angle, 0.0f, 180.0f);
    if (j.invert < 0)
        a = 180.0f - a;

    const float us = (float)j.usMin + (a / 180.0f) * (float)(j.usMax - j.usMin);
    const int32_t pulse = (int32_t)lroundf(us);

    if (pulse != _lastPulse[idx])
    {
        _pwm.writeMicroseconds(j.channel, (uint16_t)pulse);
        _lastPulse[idx] = pulse;
    }
}

// ---------------------------------------------------------------------------
//  Kalici hafiza (NVS)
// ---------------------------------------------------------------------------
void ArmController::loadPose()
{
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
        _savedPose[i] = JOINTS[i].homeDeg;

    _poseKnown = false;
    if (nvsOpen())
    {
        float tmp[NUM_JOINTS];
        const size_t n = _prefs.getBytes(NVS_POSE_KEY, tmp, sizeof(tmp));
        if (n == sizeof(tmp))
        {
            bool valid = true;
            for (uint8_t i = 0; i < NUM_JOINTS; i++)
                if (!isfinite(tmp[i]) || tmp[i] < -1.0f || tmp[i] > 181.0f)
                    valid = false;

            if (valid)
            {
                memcpy(_savedPose, tmp, sizeof(tmp));
                _poseKnown = true;
            }
        }
        nvsClose();
    }
}

void ArmController::savePose()
{
    const uint32_t now = millis();
    if (now - _lastSaveAt < MIN_SAVE_INTERVAL_MS && _lastSaveAt != 0)
        return;

    float cur[NUM_JOINTS];
    bool changed = false;
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        cur[i] = _axis[i].position();
        if (fabsf(cur[i] - _savedPose[i]) > 0.5f)
            changed = true;
    }
    if (!changed && _poseKnown)
        return;

    if (nvsOpen())
    {
        _prefs.putBytes(NVS_POSE_KEY, cur, sizeof(cur));
        nvsClose();
        memcpy(_savedPose, cur, sizeof(cur));
        _poseKnown = true;
        _lastSaveAt = now;
    }
}

// ---------------------------------------------------------------------------
//  Kalibrasyon
// ---------------------------------------------------------------------------
void ArmController::calReset()
{
    _cal.magic = CAL_MAGIC;
    for (uint8_t i = 0; i < 3; i++)
    {
        _cal.sRef[i] = CAL_DEFAULT[i].sRef;
        _cal.wRef[i] = CAL_DEFAULT[i].wRef;
        _cal.gain[i] = CAL_DEFAULT[i].gain;
    }
    _cal.gripOpen = GRIPPER_OPEN_DEG;
    _cal.gripClose = GRIPPER_CLOSE_DEG;
    _cal.parallel = CAL_PARALLEL_DEFAULT ? 1 : 0;
    calClearSamples();
}

void ArmController::loadCal()
{
    calReset();
    if (nvsOpen())
    {
        CalData tmp;
        if (_prefs.getBytes(NVS_CAL_KEY, &tmp, sizeof(tmp)) == sizeof(tmp) && tmp.magic == CAL_MAGIC)
        {
            bool ok = true;
            for (uint8_t i = 0; i < 3; i++)
            {
                const float g = fabsf(tmp.gain[i]);
                if (!isfinite(tmp.sRef[i]) || tmp.sRef[i] < -1.0f || tmp.sRef[i] > 181.0f ||
                    !isfinite(tmp.wRef[i]) || fabsf(tmp.wRef[i]) > 360.0f ||
                    !isfinite(tmp.gain[i]) || g < CAL_GAIN_MIN || g > CAL_GAIN_MAX)
                    ok = false;
            }
            if (ok)
                _cal = tmp;
        }
        nvsClose();
    }
    Serial.printf("[CAL] sRef = %.1f / %.1f / %.1f   wRef = %.1f / %.1f / %.1f\n",
                  _cal.sRef[0], _cal.sRef[1], _cal.sRef[2],
                  _cal.wRef[0], _cal.wRef[1], _cal.wRef[2]);
    Serial.printf("[CAL] gain = %+.3f / %+.3f / %+.3f   kinematik = %s\n",
                  _cal.gain[0], _cal.gain[1], _cal.gain[2],
                  _cal.parallel ? "PARALEL" : "SERI");
}

void ArmController::calSave()
{
    if (nvsOpen())
    {
        _prefs.putBytes(NVS_CAL_KEY, &_cal, sizeof(_cal));
        nvsClose();
        Serial.println("[CAL] Kaydedildi.");
    }
}

void ArmController::calCapture()
{
    // Kol su an dimdik yukari duruyor kabul edilir: omuzun dunya acisi 90,
    // dirsek ve bilek uzuvlari ayni dogrultuda oldugu icin 0. Sadece referans
    // noktasi tasinir; gain (yon/oran) bilgisine dokunulmaz.
    for (uint8_t i = 0; i < 3; i++)
    {
        _cal.sRef[i] = _axis[i + 1].position();
        _cal.wRef[i] = CAL_DEFAULT[i].wRef;
    }
    calSave();
    Serial.printf("[CAL] Dik yukari referansi yakalandi: %.1f / %.1f / %.1f\n",
                  _cal.sRef[0], _cal.sRef[1], _cal.sRef[2]);
}

void ArmController::calFlipDir(uint8_t idx)
{
    if (idx > 2)
        return;
    _cal.gain[idx] = -_cal.gain[idx];
    calSave();
}

// --- Iki noktali manuel esleme ------------------------------------------------
void ArmController::calClearSamples()
{
    _smp[0].valid = false;
    _smp[1].valid = false;
    _lastFitMask = -1;
}

bool ArmController::calSample(uint8_t slot, const float *q)
{
    if (slot > 1)
        return false;

    // Servolar torksuzken servo acisi diye bir gercek yok: kol nerede
    // birakildiysa oradadir. Boyle bir ornek kalibrasyonu bozar.
    if (_state != ARM_ACTIVE)
    {
        Serial.println("[CAL] Ornek alinamadi: kol AKTIF degil (once ETKINLESTIR).");
        return false;
    }

    for (uint8_t i = 0; i < 3; i++)
        if (!isfinite(q[i]) || fabsf(q[i]) > 360.0f)
        {
            Serial.println("[CAL] Ornek alinamadi: gecersiz aci.");
            return false;
        }

    for (uint8_t i = 0; i < 3; i++)
    {
        _smp[slot].servo[i] = _axis[i + 1].position();
        _smp[slot].q[i] = q[i];
    }
    _smp[slot].valid = true;
    _lastFitMask = -1; // yeni tur basladi: onceki hesabin raporu artik gecersiz

    Serial.printf("[CAL] Ornek %u  servo %.1f/%.1f/%.1f  ->  eklem %.1f/%.1f/%.1f\n",
                  (unsigned)(slot + 1),
                  _smp[slot].servo[0], _smp[slot].servo[1], _smp[slot].servo[2],
                  _smp[slot].q[0], _smp[slot].q[1], _smp[slot].q[2]);
    return true;
}

uint8_t ArmController::calFit()
{
    if (!_smp[0].valid || !_smp[1].valid)
    {
        Serial.println("[CAL] Fit yapilamadi: iki ornek de gerekli.");
        return 0;
    }

    uint8_t mask = 0;
    for (uint8_t i = 0; i < 3; i++)
    {
        const float ds = _smp[1].servo[i] - _smp[0].servo[i];
        const float dq = _smp[1].q[i] - _smp[0].q[i];

        // Eksen iki ornek arasinda kayda deger oynatilmamis: egim cikarilamaz.
        if (fabsf(ds) < CAL_MIN_SPAN_DEG)
        {
            Serial.printf("[CAL]   eksen %u atlandi: servo yalnizca %.1f deg oynamis "
                          "(en az %.1f gerekli)\n",
                          (unsigned)i, fabsf(ds), (double)CAL_MIN_SPAN_DEG);
            continue;
        }

        const float g = dq / ds;
        if (!isfinite(g) || fabsf(g) < CAL_GAIN_MIN || fabsf(g) > CAL_GAIN_MAX)
        {
            Serial.printf("[CAL]   eksen %u atlandi: mantiksiz kazanc %.3f\n", (unsigned)i, g);
            continue;
        }

        _cal.gain[i] = g;
        _cal.sRef[i] = _smp[0].servo[i];
        _cal.wRef[i] = _smp[0].q[i];
        mask |= (uint8_t)(1u << i);

        Serial.printf("[CAL]   eksen %u  gain %+.3f  sRef %.1f  wRef %.1f\n",
                      (unsigned)i, g, _cal.sRef[i], _cal.wRef[i]);
    }

    if (mask)
    {
        calSave();
        // Basarili fit sonrasi ornekler tuketilmis sayilir: arayuz bir sonraki
        // turu bastan baslatir, kullanici yanlislikla eski ornekle fit etmez.
        calClearSamples();
    }
    else
    {
        Serial.println("[CAL] Hicbir eksen guncellenmedi.");
    }

    _lastFitMask = (int8_t)mask; // calClearSamples'i ezer: sonucu arayuze tasir
    return mask;
}

void ArmController::calSetParallel(bool p)
{
    _cal.parallel = p ? 1 : 0;
    calSave();
}

void ArmController::calSetGripper(bool openSide)
{
    const float v = _axis[J_GRIPPER].position();
    if (openSide)
        _cal.gripOpen = v;
    else
        _cal.gripClose = v;
    calSave();
}

// ---------------------------------------------------------------------------
//  Komut arayuzu (web gorevinden cagrilir)
// ---------------------------------------------------------------------------
void ArmController::setJointTarget(uint8_t id, float deg)
{
    if (id >= NUM_JOINTS || !isfinite(deg))
        return;

    portENTER_CRITICAL(&_mux);
    _cmdTarget[id] = constrain(deg, JOINTS[id].minDeg, JOINTS[id].maxDeg);
    _cmdDirty = true;
    portEXIT_CRITICAL(&_mux);
}

void ArmController::setAllTargets(const float *deg)
{
    float clamped[NUM_JOINTS];
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
        clamped[i] = isfinite(deg[i]) ? constrain(deg[i], JOINTS[i].minDeg, JOINTS[i].maxDeg)
                                      : JOINTS[i].homeDeg;

    portENTER_CRITICAL(&_mux);
    memcpy(_cmdTarget, clamped, sizeof(clamped));
    _cmdDirty = true;
    portEXIT_CRITICAL(&_mux);
}

void ArmController::setSpeedScale(float s)
{
    if (!isfinite(s))
        return;
    _userScale = constrain(s, SPEED_SCALE_MIN, SPEED_SCALE_MAX);
    _scaleDirty = true;
}

void ArmController::snapshot(float *cur, float *tgt) const
{
    // 32-bit hizali float okumalari ESP32'de atomiktir; kilit gerekmez.
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
    {
        cur[i] = _axis[i].position();
        tgt[i] = _axis[i].target();
    }
}

// ---------------------------------------------------------------------------
//  Yardimcilar
// ---------------------------------------------------------------------------
void ArmController::syncCommandFromCurrent()
{
    float cur[NUM_JOINTS];
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
        cur[i] = _axis[i].position();

    portENTER_CRITICAL(&_mux);
    memcpy(_cmdTarget, cur, sizeof(cur));
    _cmdDirty = false;
    portEXIT_CRITICAL(&_mux);
}

void ArmController::applyScale(float s)
{
    _speedScale = s;
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
        _axis[i].setScale(s);
}

void ArmController::startSlowMove(bool toPark)
{
    applyScale(HOMING_SPEED_SCALE);
    _slowMoveActive = true;

    float pose[NUM_JOINTS];
    for (uint8_t i = 0; i < NUM_JOINTS; i++)
        pose[i] = toPark ? JOINTS[i].parkDeg : JOINTS[i].homeDeg;

    portENTER_CRITICAL(&_mux);
    memcpy(_cmdTarget, pose, sizeof(pose));
    _cmdDirty = true;
    portEXIT_CRITICAL(&_mux);
}

// ---------------------------------------------------------------------------
//  Istek kuyrugu
// ---------------------------------------------------------------------------
void ArmController::handleRequests()
{
    if (_reqStop)
    {
        _reqStop = false;
        if (_state == ARM_ACTIVE)
        {
            for (uint8_t i = 0; i < NUM_JOINTS; i++)
                _axis[i].snapTo(_axis[i].position());
            syncCommandFromCurrent();
            _slowMoveActive = false;
            applyScale(_userScale);
            _state = ARM_ESTOP;
            Serial.println("[ARM] ACIL STOP - konum korunuyor.");
        }
        else if (_state == ARM_ENGAGING)
        {
            // Devreye alma yarida kesildi: en guvenlisi tamamen serbest birakmak.
            disableOutputs();
            _state = ARM_DISARMED;
            Serial.println("[ARM] Devreye alma iptal edildi -> SERBEST.");
        }
    }

    if (_reqResume)
    {
        _reqResume = false;
        if (_state == ARM_ESTOP)
        {
            syncCommandFromCurrent();
            _state = ARM_ACTIVE;
            Serial.println("[ARM] Acil stop kaldirildi.");
        }
    }

    if (_reqRelease)
    {
        _reqRelease = false;
        if (_state != ARM_DISARMED)
        {
            savePose();
            disableOutputs();
            _state = ARM_DISARMED;
            Serial.println("[ARM] Servolar serbest birakildi (tork yok).");
        }
    }

    if (_reqEngage)
    {
        _reqEngage = false;
        if (_state == ARM_DISARMED)
        {
            // Hedef = ekranda gorulen (varsayilan) mevcut poz. Servoya once
            // TAM BU aci yazilacagi icin fiziksel sicrama olmaz.
            for (uint8_t i = 0; i < NUM_JOINTS; i++)
                _axis[i].snapTo(_axis[i].position());
            syncCommandFromCurrent();

#if PCA9685_OE_PIN >= 0
            digitalWrite(PCA9685_OE_PIN, LOW); // cikislari ac
#endif
            _engageIdx = 0;
            _engageAt = millis() - ENGAGE_STAGGER_MS;
            _state = ARM_ENGAGING;
            Serial.println("[ARM] Devreye aliniyor (kanal kanal)...");
        }
    }

    if (_reqHome)
    {
        _reqHome = false;
        if (_state == ARM_ACTIVE)
        {
            startSlowMove(false);
            Serial.println("[ARM] HOME pozuna yavas gidis.");
        }
    }

    if (_reqPark)
    {
        _reqPark = false;
        if (_state == ARM_ACTIVE)
        {
            startSlowMove(true);
            Serial.println("[ARM] PARK pozuna yavas gidis.");
        }
    }

    if (_reqSave)
    {
        _reqSave = false;
        _lastSaveAt = 0; // hemen yazmasina izin ver
        savePose();
        Serial.println("[ARM] Poz kaydedildi.");
    }
}

// ---------------------------------------------------------------------------
//  Ana dongu adimi
// ---------------------------------------------------------------------------
void ArmController::tick(float dt)
{
    handleRequests();

    if (_scaleDirty)
    {
        _scaleDirty = false;
        if (!_slowMoveActive)
            applyScale(_userScale);
    }

    // Arayuzden gelen hedefleri al
    if (_cmdDirty)
    {
        float t[NUM_JOINTS];
        portENTER_CRITICAL(&_mux);
        memcpy(t, _cmdTarget, sizeof(t));
        _cmdDirty = false;
        portEXIT_CRITICAL(&_mux);

        if (_state == ARM_DISARMED)
        {
            // Servolar serbestken kullanici "kolun su an nerede oldugunu"
            // ayarliyor. Hicbir darbe uretmeden konumu guncelliyoruz.
            for (uint8_t i = 0; i < NUM_JOINTS; i++)
                _axis[i].snapTo(t[i]);
        }
        else if (_state == ARM_ACTIVE)
        {
            for (uint8_t i = 0; i < NUM_JOINTS; i++)
                _axis[i].setTarget(t[i]);
        }
    }

    switch (_state)
    {
    case ARM_DISARMED:
        _moving = false;
        _settleRunning = false;
        break;

    case ARM_ENGAGING:
        _moving = true;
        if (millis() - _engageAt >= ENGAGE_STAGGER_MS)
        {
            writeChannel(_engageIdx, _axis[_engageIdx].position());
            Serial.printf("[ARM]   kanal %u -> %.1f deg\n",
                          JOINTS[_engageIdx].channel, _axis[_engageIdx].position());
            _engageAt = millis();
            _engageIdx++;

            if (_engageIdx >= NUM_JOINTS)
            {
                _state = ARM_ACTIVE;
                _moving = false;
                Serial.println("[ARM] AKTIF.");
                if (_pendingHomeAfterEngage)
                {
                    _pendingHomeAfterEngage = false;
                    startSlowMove(false);
                }
            }
        }
        break;

    case ARM_ACTIVE:
    {
        bool moving = false;
        for (uint8_t i = 0; i < NUM_JOINTS; i++)
        {
            if (_axis[i].update(dt))
                moving = true;
            writeChannel(i, _axis[i].position());
        }
        _moving = moving;

        if (_slowMoveActive && !moving)
        {
            _slowMoveActive = false;
            applyScale(_userScale);
        }

        // Hareket bittikten bir sure sonra pozu kalici hafizaya yaz
        if (moving)
        {
            _settleRunning = false;
        }
        else if (!_settleRunning)
        {
            _settleRunning = true;
            _settledAt = millis();
        }
        else if (millis() - _settledAt > IDLE_SAVE_DELAY_MS)
        {
            savePose();
            _settledAt = millis();
        }
        break;
    }

    case ARM_ESTOP:
        // Konum korunur: son darbe degeri PCA9685'te duruyor, servolar tutuyor.
        _moving = false;
        break;
    }
}
