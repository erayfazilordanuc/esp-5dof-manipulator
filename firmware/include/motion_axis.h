/**
 * ArmPilot - Tek eksen hareket profili uretici
 * ---------------------------------------------------------------------------
 * Servo motorda konum geri beslemesi YOKTUR; dolayisiyla klasik bir PID
 * calistirilamaz. Bunun yerine "profil takibi" yapiyoruz: hedefe giden yolu
 * hiz / ivme / jerk sinirlarina uyan yumusak bir egri olarak uretip servoya
 * her adimda ARA konum yaziyoruz. Sonuc PID'li bir eksen gibi davranir:
 *
 *      hiz
 *       ^      ______________
 *       |     /              \        <- jerk limiti kalkisi yumusatir
 *       |    /                \       <- sqrt(2*a*mesafe) freni asmayi onler
 *       +---/------------------\----> zaman
 *
 * Ozellikler:
 *  - Kalkista jerk limiti  => "yavas basla, sonra hizlan"
 *  - Frende tam ivme serbest => hedefte asma (overshoot) olmaz
 *  - Hareket sirasinda hedef degisse bile profil kesintisiz devam eder
 */
#pragma once

#include <Arduino.h>

class MotionAxis
{
public:
    void begin(float startPos, float vMax, float aMax, float jMax, float lo, float hi)
    {
        _lo = lo;
        _hi = hi;
        _vMax = vMax;
        _aMax = aMax;
        _jMax = jMax;
        snapTo(startPos);
    }

    /** Profili sifirlayarak konumu zorla (kilitlenme, acil stop, arm etme). */
    void snapTo(float pos)
    {
        _pos = constrain(pos, _lo, _hi);
        _tgt = _pos;
        _vel = 0.0f;
        _acc = 0.0f;
    }

    void setTarget(float t) { _tgt = constrain(t, _lo, _hi); }
    void setScale(float s) { _scale = constrain(s, 0.05f, 2.0f); }

    float position() const { return _pos; }
    float target() const { return _tgt; }
    float velocity() const { return _vel; }
    float lowerLimit() const { return _lo; }
    float upperLimit() const { return _hi; }

    bool atTarget() const
    {
        return fabsf(_tgt - _pos) < POS_EPS && fabsf(_vel) < VEL_EPS;
    }

    /**
     * Profili dt kadar ilerletir.
     * @return true ise eksen hala hareket halinde.
     */
    bool update(float dt)
    {
        // Hiz olcegi dususunce ivme ve jerk de dussun ki hareketin karakteri
        // (yumusakligi) korunsun, sadece suresi uzasin.
        const float vMax = _vMax * _scale;
        const float aMax = _aMax * _scale * _scale;
        const float jMax = _jMax * _scale * _scale * _scale;

        const float err = _tgt - _pos;

        // Hedefteyiz ve durmusuz -> profili kilitle
        if (fabsf(err) < POS_EPS && fabsf(_vel) < VEL_EPS)
        {
            _pos = _tgt;
            _vel = 0.0f;
            _acc = 0.0f;
            return false;
        }

        // 1) Kalan mesafede guvenle durabilecegimiz en yuksek hiz (fren egrisi)
        const float vBrake = sqrtf(2.0f * aMax * fabsf(err));
        const float vCmd = (err >= 0.0f ? 1.0f : -1.0f) * min(vMax, vBrake);

        // 2) Bu hiza ulasmak icin gereken ivme, ivme siniri icinde
        float aCmd = (vCmd - _vel) / dt;
        aCmd = constrain(aCmd, -aMax, aMax);

        // 3) Jerk limiti SADECE hizlanirken uygulanir.
        //    Yavaslarken tam ivme serbest kalir; boylece asla hedefi asmayiz.
        const bool speedingUp = (fabsf(vCmd) > fabsf(_vel)) && (vCmd * _vel >= 0.0f);
        if (speedingUp)
        {
            const float dA = constrain(aCmd - _acc, -jMax * dt, jMax * dt);
            _acc += dA;
        }
        else
        {
            _acc = aCmd;
        }

        // 4) Entegrasyon
        _vel = constrain(_vel + _acc * dt, -vMax, vMax);
        _pos += _vel * dt;

        // 5) Sayisal artiklardan dogabilecek asmayi kirp
        if ((err > 0.0f && _pos > _tgt) || (err < 0.0f && _pos < _tgt))
        {
            _pos = _tgt;
            _vel = 0.0f;
            _acc = 0.0f;
        }
        _pos = constrain(_pos, _lo, _hi);
        return true;
    }

private:
    static constexpr float POS_EPS = 0.05f; // derece
    static constexpr float VEL_EPS = 0.50f; // derece/s

    float _pos = 90.0f;
    float _tgt = 90.0f;
    float _vel = 0.0f;
    float _acc = 0.0f;

    float _vMax = 60.0f;
    float _aMax = 120.0f;
    float _jMax = 600.0f;
    float _scale = 1.0f;

    float _lo = 0.0f;
    float _hi = 180.0f;
};
