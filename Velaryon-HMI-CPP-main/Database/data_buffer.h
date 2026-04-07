#pragma once
#include <QVector>
#include <QDateTime>
#include <cstdint>

// Un enregistrement = un paquet de données provenant de la Teensy dans la base de données + temps réel
struct DataRecord {
    QDateTime timestamp;      // Temps réel calculé
    int       is_test;        // 0=idle, 1=test, 2=urgence (= Data.state)
    int       test_id;        // ID du test actif dans la DB, -1 si aucun
    uint32_t  packet_id;      // Data.n
    uint32_t  teensy_millis;  // Data.t brut

    // Les 36 "capteurs" (valeurs brutes type variables, comme dans la Teensy)
    int16_t  PS11, PS12, PS21, PS22, PS23;
    int32_t  PS31;
    int16_t  PS41, PS42;
    int32_t  PS51;
    int16_t  PS61, PS62, PS63, PS64, PS71, PS81;
    int16_t  TS11, TS12, TS41, TS42, TS61, TS62;
    uint16_t FM11, FM21, FM61;
    int32_t  LC;
    uint16_t ref5V;
    uint16_t glowplug;
    uint32_t valvesState;
    uint8_t  actLPos, actRPos;
    uint8_t  actLOK, actROK;
    uint8_t  state;
    uint8_t  test_step;
    bool     test_cooling;
};

class DataBuffer {
public:
    static constexpr int BUFFER_SIZE = 500; //définit une taille constante pour le buffer dès la compilation

    void      push(const DataRecord& record);  // Ajoute un enregistrement
    bool      isFull() const;
    int       size() const;
    void      clear();
    const QVector<DataRecord>& records() const;

private:
    QVector<DataRecord> m_records;
};
