/**
 * ArmPilot - Kol kontrolcusu
 * ---------------------------------------------------------------------------
 * Sorumluluklari:
 *   - PCA9685 surumu ve servo darbe uretimi
 *   - Durum makinesi (SERBEST / DEVREYE ALINIYOR / AKTIF / ACIL STOP)
 *   - Guvenli acilis: acilista HICBIR darbe uretilmez, servolar serbest kalir
 *   - Son pozun NVS'e kaydi ve acilista geri yuklenmesi (sicramayi onler)
 *   - Hareket profillerinin islenmesi
 *
 * Iplik guvenligi: komutlar web (AsyncTCP) gorevinden gelir, hareket dongusu
 * ayri bir FreeRTOS gorevinde kosar. Hedefler spinlock korumali bir tampon
 * uzerinden, durum degisiklikleri ise "istek bayraklari" ile aktarilir.
 */
#pragma once

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <Preferences.h>

#include "config.h"
#include "motion_axis.h"

enum ArmState : uint8_t
{
    ARM_DISARMED = 0, // Servolar serbest (tork yok) - guvenli acilis durumu
    ARM_ENGAGING = 1, // Kanallar tek tek devreye aliniyor
    ARM_ACTIVE = 2,   // Normal calisma
    ARM_ESTOP = 3     // Acil stop: konum korunuyor, hareket yok
};

/**
 * NVS'te saklanan kalibrasyon bloku.
 * Her eklem icin: eklem_acisi = wRef + gain*(servo - sRef)   (bkz. config.h)
 */
struct CalData
{
    uint16_t magic;
    float sRef[3]; // omuz / dirsek / bilek: ornek noktadaki servo acisi
    float wRef[3]; // ayni noktadaki gercek eklem acisi
    float gain[3]; // eklem acisi / servo acisi (isaret = yon)
    float gripOpen;
    float gripClose;
    uint8_t parallel;
};
// Blok duzeni v1'den farkli; eski kayit okunmaz, fabrika ayarina duser.
#define CAL_MAGIC 0xA52C

/**
 * Kalibrasyon modunda alinan tek bir ornek: kullanicinin arayuzde gercek kola
 * benzettigi eklem acilari + o andaki servo acilari. Yalnizca RAM'de durur.
 */
struct CalSample
{
    float servo[3];
    float q[3];
    bool valid;
};

class ArmController
{
public:
    void begin();

    /** Hareket gorevi tarafindan sabit periyotla cagrilir. */
    void tick(float dt);

    // --- Komutlar (web gorevinden guvenle cagrilabilir) ---
    void setJointTarget(uint8_t id, float deg);
    void setAllTargets(const float *deg);
    void setSpeedScale(float s);
    void requestEngage() { _reqEngage = true; }
    void requestRelease() { _reqRelease = true; }
    void requestHome() { _reqHome = true; }
    void requestPark() { _reqPark = true; }
    void requestStop() { _reqStop = true; }
    void requestResume() { _reqResume = true; }
    void requestSave() { _reqSave = true; }

    // --- Kalibrasyon ---
    /** Hizli yol: mevcut pozu "dik yukari" kabul edip referansi tasir (gain'e dokunmaz). */
    void calCapture();
    void calFlipDir(uint8_t idx);        // idx 0..2, gain'in isaretini cevirir
    void calSetParallel(bool p);
    void calSetGripper(bool openSide);   // true: tam acik, false: tam kapali
    void calReset();
    void calSave();
    const CalData &cal() const { return _cal; }

    // --- Kalibrasyon modu (iki noktali manuel esleme) ---
    /**
     * Kullanicinin arayuzde gercek kola benzettigi eklem acilarini, o andaki
     * servo acilariyla birlikte kaydeder. Yalnizca kol AKTIF'ken anlamlidir:
     * servolar torksuzken "servo acisi" diye bir gercek yoktur.
     * @return ornek alindiysa true
     */
    bool calSample(uint8_t slot, const float *q); // slot 0 veya 1
    /**
     * Iki ornekten her eksenin gain/sRef/wRef degerlerini cozer ve kaydeder.
     * Yeterince oynatilmamis eksenler atlanir (eski degeri korunur).
     * @return guncellenen eksenlerin bit maskesi (bit0 omuz .. bit2 bilek)
     */
    uint8_t calFit();
    void calClearSamples();
    bool calSampleValid(uint8_t slot) const { return slot < 2 && _smp[slot].valid; }
    const CalSample &calSampleData(uint8_t slot) const { return _smp[slot < 2 ? slot : 0]; }
    /** Son calFit sonucu: -1 hic denenmedi, aksi halde guncellenen eksen maskesi. */
    int8_t lastFitMask() const { return _lastFitMask; }

    // --- Durum sorgulari ---
    ArmState state() const { return _state; }
    bool isMoving() const { return _moving; }
    bool poseKnown() const { return _poseKnown; }
    float speedScale() const { return _userScale; }
    void snapshot(float *cur, float *tgt) const;

private:
    void loadCal();
    void writeChannel(uint8_t idx, float angle);
    void disableOutputs();
    void loadPose();
    void savePose();
    void handleRequests();
    void syncCommandFromCurrent();
    void applyScale(float s);
    void startSlowMove(bool toPark);

    Adafruit_PWMServoDriver _pwm{PCA9685_ADDR};
    MotionAxis _axis[NUM_JOINTS];
    Preferences _prefs;
    CalData _cal{};
    CalSample _smp[2]{};
    int8_t _lastFitMask = -1;

    volatile ArmState _state = ARM_DISARMED;
    volatile bool _moving = false;
    bool _poseKnown = false;

    float _speedScale = SPEED_SCALE_DEFAULT; // Eksenlere uygulanan aktif olcek
    float _userScale = SPEED_SCALE_DEFAULT;  // Kullanicinin sectigi olcek
    bool _slowMoveActive = false;            // Home/Park sirasinda gecici yavas mod

    int32_t _lastPulse[NUM_JOINTS] = {-1, -1, -1, -1, -1};
    float _savedPose[NUM_JOINTS] = {90, 90, 90, 90, 90};
    float _cmdTarget[NUM_JOINTS] = {90, 90, 90, 90, 90};

    uint8_t _engageIdx = 0;
    uint32_t _engageAt = 0;
    uint32_t _settledAt = 0;
    uint32_t _lastSaveAt = 0;
    bool _settleRunning = false;
    bool _pendingHomeAfterEngage = false;

    volatile bool _cmdDirty = false;
    volatile bool _scaleDirty = false;
    volatile bool _reqEngage = false;
    volatile bool _reqRelease = false;
    volatile bool _reqHome = false;
    volatile bool _reqPark = false;
    volatile bool _reqStop = false;
    volatile bool _reqResume = false;
    volatile bool _reqSave = false;

    mutable portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

    // Poz kaydi hareket gorevinden, kalibrasyon kaydi web gorevinden gelir;
    // Preferences nesnesi paylasildigi icin erisim mutex ile serilestirilir.
    SemaphoreHandle_t _nvsLock = nullptr;
    bool nvsOpen();
    void nvsClose();
};

extern ArmController arm;
