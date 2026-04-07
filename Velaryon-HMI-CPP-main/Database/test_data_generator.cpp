#include "Database/test_data_generator.h"
#include <QDebug>
#include <cmath>
#include <cstdlib>

//  Bruit simplifié
//  Retourne baseValue * (1 + N(0, amplitudePct/100))
// Donc change baseValue selon le pourcentage d'amplitude souhaité (ex : 2 % → amplitudePct=2.0)
double TestDataGenerator::noise(double baseValue, double amplitudePct)
{
    // u = Somme de 12 tirage aléatoires entre 0 et 1 → une distribution uniforme
    double u = 0.0;
    for (int i = 0; i < 12; ++i)
        u += static_cast<double>(std::rand()) / RAND_MAX; //Ici u est égal envrion à 6
    u -= 6.0;   //Donc on retire 6 pour le recentrer à 0, ce qui mathématiquement correspond à un nombre pseudo aléatoire gaussien (u compris entre 0 et 1)

    return baseValue * (1.0 + u * amplitudePct / 100.0); //On retourne la baseValue - (u * notre pourcentage d'amplitude) donc on à du bruit.
}

//  Construit un faux enregistrement pour un nombre de paquet donné.
DataRecord TestDataGenerator::makeFakeRecord(int packetIdx, int testPacketIdx, int testId, bool isTest)
{
    DataRecord r{};

    // Timestamp : T0 = maintenant, +50 ms par paquet (~20 Hz)
    static QDateTime t0 = QDateTime::currentDateTime();
    r.timestamp     = t0.addMSecs(static_cast<qint64>(packetIdx) * 50);
    r.teensy_millis = static_cast<uint32_t>(packetIdx * 50);
    r.packet_id     = static_cast<uint32_t>(packetIdx);
    r.is_test       = isTest ? 1 : 0;
    r.test_id       = testId;
    r.state         = static_cast<uint8_t>(isTest ? 1 : 0);

    // Progression dans la phase de test (0.0 → 1.0)
    // FIX: utilise testPacketIdx (index LOCAL dans la phase de test) et non
    // packetIdx (index global). Avant ce correctif, si 500 paquets idle
    // précédaient le test, le premier paquet de test avait déjà progress=1.0
    // et la montée en pression/température n'était pas simulée du tout.
    double progress = isTest ? std::min(1.0, testPacketIdx / 500.0) : 0.0;

    // ---- Pressions (brutes, unité : mbar × 1 = entiers 0..16 000) ----
    // LOX : monte de 0 à 12 000 mbar pendant le test
    r.PS11 = static_cast<int16_t>(noise(progress * 12000.0, 2.0));
    r.PS12 = static_cast<int16_t>(noise(progress * 11500.0, 2.0));

    // ETH : monte de 0 à 10 000 mbar
    r.PS21 = static_cast<int16_t>(noise(progress * 10000.0, 2.0));
    r.PS22 = static_cast<int16_t>(noise(progress *  9800.0, 2.0));
    r.PS23 = static_cast<int16_t>(noise(progress *  9500.0, 2.0));

    // GN2 : monte de 0 à 200 000 mbar (int32)
    r.PS31 = static_cast<int32_t>(noise(progress * 200000.0, 2.0));

    // Chambre de combustion : monte de 0 à 16 000 mbar
    r.PS41 = static_cast<int16_t>(noise(progress * 16000.0, 2.0));
    r.PS42 = static_cast<int16_t>(noise(progress * 15800.0, 2.0));

    // Air : int32, jusqu'à 800 000 mbar
    r.PS51 = static_cast<int32_t>(noise(progress * 800000.0, 2.0));

    // Eau refroidissement : stable ~5 000 mbar pendant le test
    r.PS61 = static_cast<int16_t>(noise(isTest ? 5000.0 : 100.0, 2.0));
    r.PS62 = static_cast<int16_t>(noise(isTest ? 4900.0 : 100.0, 2.0));
    r.PS63 = static_cast<int16_t>(noise(isTest ? 4800.0 : 100.0, 2.0));
    r.PS64 = static_cast<int16_t>(noise(isTest ? 4700.0 : 100.0, 2.0));
    r.PS71 = static_cast<int16_t>(noise(100.0, 3.0));
    r.PS81 = static_cast<int16_t>(noise(100.0, 3.0));

    // ---- Températures (brutes, unité : °C × 10) ----
    // TS11/TS12 : ambiantes (~20 °C → 200 brut)
    r.TS11 = static_cast<int16_t>(noise(200.0, 1.0));
    r.TS12 = static_cast<int16_t>(noise(200.0, 1.0));

    // TS41/TS42 : légère montée pendant le test
    r.TS41 = static_cast<int16_t>(noise(200.0 + progress * 300.0, 2.0));
    r.TS42 = static_cast<int16_t>(noise(200.0 + progress * 250.0, 2.0));

    // TS61 : thermocouple chambre → monte de 20 °C à 1 200 °C (×10 : 200 → 12 000)
    r.TS61 = static_cast<int16_t>(noise(200.0 + progress * 11800.0, 2.0));
    r.TS62 = static_cast<int16_t>(noise(200.0 + progress *  5000.0, 2.0));

    // ---- Débitmètres (bruts, mL/s) ----
    r.FM11 = static_cast<uint16_t>(noise(isTest ? 250.0 : 0.0, 3.0));  // LOX
    r.FM21 = static_cast<uint16_t>(noise(isTest ? 180.0 : 0.0, 3.0));  // ETH
    r.FM61 = static_cast<uint16_t>(noise(isTest ? 500.0 : 0.0, 2.0));  // Eau refroidissement

    // ---- Capteur de Force (N) ----
    r.LC = static_cast<int32_t>(noise(isTest ? progress * 5000.0 : 0.0, 2.0));

    // ---- Tension de référence 5 V (mV × 10 000 → ~50 000 brut) ----
    r.ref5V = static_cast<uint16_t>(noise(49800.0, 0.5));

    // ---- Courant bougie (mA, brut) ----
    r.glowplug = static_cast<uint16_t>(isTest && progress < 0.05 ? noise(8000.0, 5.0) : 0);

    // ---- États vannes (bitmap 20 bits) ----
    // Pendant le test : quelques vannes ouvertes (bits 0, 2, 5)
    r.valvesState = isTest ? 0b00000000000000100101u : 0u;

    // ---- Actionneurs ----
    r.actLPos = static_cast<uint8_t>(isTest ? 90 : 0);
    r.actRPos = static_cast<uint8_t>(isTest ? 90 : 0);
    r.actLOK  = 1;
    r.actROK  = 1;

    // ---- Étape de séquence ----
    r.test_step    = static_cast<uint8_t>(isTest ? static_cast<int>(progress * 10) : 0);
    r.test_cooling = isTest && progress > 0.8;  // refroidissement en fin de test

    return r;
}

//  Génère une session fictive complète et l'insère dans la DB.
void TestDataGenerator::generateFakeSession(DatabaseManager* db,
                                             int  nPackets,
                                             bool includeTest)
{
    if (!db || !db->isOpen()) {
        qWarning() << "[TestDataGenerator] DB non ouverte, abandon.";
        return;
    }

    std::srand(42);  // on utilise une seed pour random fixe → résultats reproductibles

    // Répartition des paquets
    const int idleBeforeCount = nPackets / 4;           // 25 % idle avant
    const int testCount       = includeTest ? nPackets / 2 : 0; // 50 % test
    const int idleAfterCount  = nPackets - idleBeforeCount - testCount; // reste idle après

    qDebug() << "[TestDataGenerator] Génération de" << nPackets << "paquets :";
    qDebug() << "  idle avant :" << idleBeforeCount
             << "| test :"       << testCount
             << "| idle après :" << idleAfterCount;

    int packetIdx = 0;

    // ---- Phase 1 : idle avant le test ----
    for (int i = 0; i < idleBeforeCount; ++i) {
        // testPacketIdx = 0 pour les paquets idle (progress sera 0.0 de toute façon)
        db->onNewPacket(makeFakeRecord(packetIdx++, 0, -1, false)); 
    }

    // ---- Phase 2 : test ----
    int testId = -1;
    if (includeTest) {
        testId = db->startTest("fake_sequence.txt");
        qDebug() << "[TestDataGenerator] Test démarré, id =" << testId;

        for (int i = 0; i < testCount; ++i) {
            // FIX: on passe 'i' comme index LOCAL dans le test (0 au 1er paquet de test)
            // plutôt que packetIdx (global), ce qui garantit une vraie montée de 0→1
            db->onNewPacket(makeFakeRecord(packetIdx++, i, testId, true));
        }

        db->endTest(testId);
        qDebug() << "[TestDataGenerator] Test terminé.";
    }

    // ---- Phase 3 : idle après le test ----
    for (int i = 0; i < idleAfterCount; ++i) {
        db->onNewPacket(makeFakeRecord(packetIdx++, 0, -1, false));
    }

    // Flush final pour s'assurer que tout est en DB
    db->flushBuffer();

    qDebug() << "[TestDataGenerator] Session fictive insérée :" << packetIdx << "paquets.";
}
