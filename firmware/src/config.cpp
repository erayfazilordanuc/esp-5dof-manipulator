/**
 * ArmPilot - Yapilandirma tablolari
 */
#include "config.h"

// ---------------------------------------------------------------------------
//  EKSEN TABLOSU
//  Taban ve omuz kasitli olarak yavas: en buyuk atalet/yuk onlarda.
//  Uca dogru gidildikce hafifledigi icin hizlar artiyor.
// ---------------------------------------------------------------------------
//        isim          kanal  min   max   home  park   vMax   aMax   jMax   usMin usMax inv
const JointConfig JOINTS[NUM_JOINTS] = {
    {"Taban", 0, 0.0f, 180.0f, 90.0f, 90.0f, 40.0f, 55.0f, 300.0f, 500, 2500, +1},
    {"Omuz", 1, 5.0f, 175.0f, 90.0f, 125.0f, 30.0f, 42.0f, 220.0f, 500, 2500, +1},
    {"Dirsek", 2, 0.0f, 180.0f, 90.0f, 155.0f, 70.0f, 110.0f, 700.0f, 500, 2500, +1},
    {"Bilek", 3, 0.0f, 180.0f, 90.0f, 120.0f, 95.0f, 160.0f, 1000.0f, 500, 2500, +1},
    {"Kiskac", 4, 0.0f, 180.0f, 60.0f, 60.0f, 130.0f, 260.0f, 1800.0f, 500, 2500, +1},
};

// ---------------------------------------------------------------------------
//  GEOMETRI (mm) - CAD olculerinle degistir
// ---------------------------------------------------------------------------
const ArmGeometry GEOMETRY = {
    62.0f,  // baseRadius
    105.0f, // baseHeight  (zemin -> omuz ekseni)
    110.0f, // linkUpperArm (omuz -> dirsek)
    95.0f,  // linkForearm  (dirsek -> bilek)
    72.0f   // linkGripper  (bilek -> kiskac ucu)
};

// ---------------------------------------------------------------------------
//  KALIBRASYON - FABRIKA AYARI
//    eklem_acisi = wRef + gain*(servo - sRef)
//  Asagidaki sRef degerleri kol DIMDIK YUKARI dururken olculen servo
//  acilaridir; o pozda omuzun dunya acisi 90, dirsek ve bilek ise uzuvlar
//  ayni dogrultuda oldugu icin 0'dir. |gain| = 1 -> dogrudan tahrik; isaret
//  yonu tasir: bilek servosu ters monte oldugu icin -1.
//  Arayuzdeki KALIBRASYON MODU bu tabloyu ezip NVS'e yazar.
// ---------------------------------------------------------------------------
//                sRef    wRef    gain
const JointCal CAL_DEFAULT[3] = {
    {53.4f, 90.0f, +1.0f},  // Omuz   (dunya acisi)
    {156.4f, 0.0f, +1.0f},  // Dirsek (bir onceki uzva gore)
    {101.5f, 0.0f, -1.0f}   // Bilek  (bir onceki uzva gore) - ters monte
};
