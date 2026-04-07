#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QVector>
#include <QDateTime>
#include <QString>

#include "data_buffer.h"

// ============================================================
//  DatabaseManager
//  Gere la base de donnees SQLite pour l'acquisition Velaryon.
//
//  Responsabilites :
//    - Ouvrir / creer la DB (velaryon_data.db)
//    - Creer les tables si elles n'existent pas
//    - Accumuler les paquets dans un DataBuffer (500 paquets)
//    - Flusher le buffer vers la DB en une seule transaction
//    - Demarrer / terminer une session de test
//    - Purger les donnees "idle" (hors test)
//    - Fournir la liste des tests a l'onglet Extraction
//    - Exporter en CSV depuis la DB
// ============================================================

class DatabaseManager : public QObject
{
    Q_OBJECT
    public:
    // ---- Informations d'un test a donner pour l'onglet Extraction ----
    struct TestInfo {
        int     id;           // Cle primaire dans la table tests = +1 a chaque ligne, "mesure" dans la DB
        QString name;         // Nom genere automatiquement
        QString startedAt;    // type "ISO8601" : "2026-03-26T14:30:22.123"
        QString endedAt;      // Vide si le test est encore en cours
        QString sequenceFile; // Fichier sequence utilise (peut etre vide)
    };

    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    // Ouvre (ou cree) le fichier DB au chemin indique, retourne true si succes.
    bool openDatabase(const QString& path);

    void closeDatabase();

    bool isOpen() const;

    //  "Point d'entree" principal : appele par UdpReceiver a chaque paquet Teensy recu.
    void onNewPacket(const DataRecord& record);

    // Cree une nouvelle ligne dans la table `tests` --> celle avec les mesures constantes
    // Est appele au demarrage d'un test (launch_test = true).
    // Retourne l'ID de la ligne creee (-1 si erreur).
    int  startTest(const QString& sequenceFile = "");

    // Met a jour `ended_at` pour marquer la fin du test.
    // Est appele quand end_test devient true.
    void endTest(int testId);

    // Retourne l'ID du test actuellement actif (-1 si aucun).
    int  currentTestId() const { return m_currentTestId; }

    // Detecte les tests (ended_at NULL car crash IHM) et les cloture avec le marqueur INTERRUPTED:.
    // A appeler une seule fois au demarrage, avant toute reception UDP.
    void closeOrphanTests();

    // Vide le buffer et insere immediatement en DB. Appele a la fermeture de l'IHM ou sur demande.
    void flushBuffer();

    // Supprime toutes les mesures collectees HORS test (is_test = 0).
    // Les donnees de tests sont conservees.
    void clearIdleData();

    // Retourne la liste de tous les tests enregistres (du plus recent au plus ancien).
    QVector<TestInfo> getTestList();

    // Retourne la liste complete des noms de colonnes capteurs presents dans la DB.
    static QStringList allSensorColumns();

    // Retourne le nombre de lignes de donnees correspondant aux criteres (meme logique que exportToCsv).
    // Utilise pour detecter un export vide avant d'ouvrir le selecteur de fichier.
    int countData(int              testId = -1,
                  const QDateTime& from   = QDateTime(),
                  const QDateTime& to     = QDateTime());

    // Exporte en CSV les capteurs demandes dans Extraction, pour un test ou une plage temporelle donnes.
    // Retourne true si l'export s'est bien passe.
    bool exportToCsv(const QString&     outputPath,
                     const QStringList& sensors,
                     int                testId = -1,
                     const QDateTime&   from   = QDateTime(),
                     const QDateTime&   to     = QDateTime());

    signals:
    // Emis en cas d'erreur SQL (pour affichage dans le log terminal).
    void databaseError(const QString& message);
    void testListChanged();  // Emis quand un test est cree ou termine

    private:

    // Cree les tables SQL de la DB si elles n'existent pas encore.
    void createTables();

    // Insere un vecteur d'enregistrements en une seule transaction --> fonction de base
    void flushToDatabase(const QVector<DataRecord>& records);

    // Construit la clause WHERE pour exportToCsv.
    QString buildWhereClause(int testId, const QDateTime& from, const QDateTime& to);

    // ---- Membres prives ----

    QSqlDatabase m_db;           // Connexion SQLite
    DataBuffer   m_buffer;       // Buffer de 500 paquets
    QTimer*      m_flushTimer;   // Flush auto toutes les 2 secondes
    int          m_currentTestId = -1; // ID du test actif (-1 = idle)
};
