// ABANDONNÉ — Remplacé par le simulateur Python sender_test.
// Conservé comme référence mais retiré du build.
#pragma once
#include "Database/database_manager.h"
#include "Database/data_buffer.h"

// ============================================================
//  TestDataGenerator
//  Génère une session fictive complète et l'insère en DB.
//  Sert uniquement à valider le pipeline DB sans avoir le
//  Teensy branché. --> SERT DE DEBUG
//
//  Scénario simulé (2 000 paquets par défaut) :
//    - 500 paquets idle (is_test = 0)
//    - Démarrage du test → pressions montent, TS61 chauffe, FM11 non nul
//    - 1 000 paquets de test (is_test = 1)
//    - Fin du test
//    - 500 paquets idle supplémentaires (is_test = 0)
//
//  Toutes les valeurs sont bruitées de base légèrement.
// ============================================================

class TestDataGenerator {
public:
    // Point d'entrée principal.
    // db          : DatabaseManager déjà ouvert
    // nPackets    : nombre total de paquets à générer (idle + test + idle)
    // includeTest : si false, tous les paquets sont idle (pour tester la purge)
    static void generateFakeSession(DatabaseManager* db,
                                    int  nPackets    = 2000,
                                    bool includeTest = true);

private:
    // Construit un faux enregistrement pour un nombre de paquet donné.
    // packetIdx    : index global du paquet (pour le timestamp et packet_id)
    // testPacketIdx: index LOCAL dans la phase de test (0 = premier paquet de test)
    //                Utilisé pour calculer la progression (0→1) sans biais dû aux paquets idle.
    // testId       : ID de la session de test dans la DB (-1 si idle)
    // isTest       : true si le paquet appartient à la phase de test
    static DataRecord makeFakeRecord(int packetIdx, int testPacketIdx, int testId, bool isTest);

    // Bruit autour de la valeur de base
    static double noise(double baseValue, double amplitudePct);
};
