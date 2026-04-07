#include "database_manager.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QTextStream>
#include <QDebug>

// ============================================================
//  Constructeur / Destructeur
// ============================================================

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , m_flushTimer(new QTimer(this))
{
    // Flush automatique toutes les 2 secondes,
    // même si le buffer n'est pas encore plein.
    // Évite de perdre des données si le flux est lent.
    connect(m_flushTimer, &QTimer::timeout,
            this,          &DatabaseManager::flushBuffer);
    m_flushTimer->setInterval(2000);
    m_flushTimer->start();
}

DatabaseManager::~DatabaseManager()
{
    // On flush ce qu'il reste avant de fermer.
    flushBuffer();
    closeDatabase();
}

bool DatabaseManager::openDatabase(const QString& path)
{
    // On utilise un nom de connexion unique pour éviter les conflits
    // si jamais plusieurs instances existaient (rare, mais prudent).
    m_db = QSqlDatabase::addDatabase("QSQLITE", "velaryon_connection");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        emit databaseError("Impossible d'ouvrir la DB : " + m_db.lastError().text());
        return false;
    }

    // Active le mode WAL (Write-Ahead Logging) de SQLite :
    // améliore les performances en écriture sans bloquer les lectures.
    QSqlQuery pragmaQuery(m_db);
    pragmaQuery.exec("PRAGMA journal_mode = WAL"); // Permet l'écriture sans bloquer les lectures de données
    pragmaQuery.exec("PRAGMA synchronous = NORMAL"); // Baisse de sécuritée  (si crash risque de corruption du dernier paquet mais améliore les perf)

    createTables();
    qDebug() << "[DB] Base ouverte :" << path;
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    // Qt demande qu'on retire la connexion après fermeture.
    QSqlDatabase::removeDatabase("velaryon_connection");
}

bool DatabaseManager::isOpen() const // Vérifie si la DB est ouverte
{
    return m_db.isOpen();
}

void DatabaseManager::createTables()
{
    QSqlQuery q(m_db);

    // ---- Table des sessions de test ----
    // Chaque test correspond à une exécution de séquence. Ne contient que des infos "constantes" du test (nom, horaires, fichier séquence), pas les mesures.
    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS tests (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            name          TEXT    NOT NULL,
            started_at    TEXT    NOT NULL,
            ended_at      TEXT,
            sequence_file TEXT
        )
    )");
    if (!ok)
        emit databaseError("Erreur création table tests : " + q.lastError().text());

    // ---- Table des mesures ----
    // is_test : 0 = idle (hors test), 1 = test actif, 2 = urgence
    // test_id : NULL si hors test
    // Les valeurs sont stockées brutes (comme reçues du Teensy).
    // Les facteurs d'échelle sont appliqués seulement à l'export CSV.
    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS sensor_data (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            test_id       INTEGER REFERENCES tests(id),
            is_test       INTEGER NOT NULL DEFAULT 0,
            timestamp     TEXT    NOT NULL,
            packet_id     INTEGER,
            teensy_millis INTEGER,

            PS11 INTEGER, PS12 INTEGER,
            PS21 INTEGER, PS22 INTEGER, PS23 INTEGER,
            PS31 INTEGER,
            PS41 INTEGER, PS42 INTEGER,
            PS51 INTEGER,
            PS61 INTEGER, PS62 INTEGER, PS63 INTEGER, PS64 INTEGER,
            PS71 INTEGER, PS81 INTEGER,

            TS11 INTEGER, TS12 INTEGER,
            TS41 INTEGER, TS42 INTEGER,
            TS61 INTEGER, TS62 INTEGER,

            FM11 INTEGER, FM21 INTEGER, FM61 INTEGER,

            LC       INTEGER,
            ref5V    INTEGER,
            glowplug INTEGER,

            valvesState INTEGER,
            actLPos INTEGER, actRPos INTEGER,
            actLOK  INTEGER, actROK  INTEGER,
            state   INTEGER,
            test_step INTEGER,
            test_cooling INTEGER
        )
    )");
    if (!ok)
        emit databaseError("Erreur création table sensor_data : " + q.lastError().text());

    // ---- Index pour accélérer les requêtes de l'onglet Extraction ----
    q.exec("CREATE INDEX IF NOT EXISTS idx_sensor_test_id   ON sensor_data(test_id)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_sensor_timestamp ON sensor_data(timestamp)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_sensor_is_test   ON sensor_data(is_test)");
}

//orphan tests = tests qui n'ont pas de timestamp de fin (ended_at) car l'IHM a crashé pendant le test sans pouvoir marquer la fin du test en DB.
void DatabaseManager::closeOrphanTests()
{
    if (!m_db.isOpen()) return;

    QString now = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    QSqlQuery q(m_db);
    q.prepare("UPDATE tests SET ended_at = :ended_at WHERE ended_at IS NULL");
    q.bindValue(":ended_at", "INTERRUPTED:" + now);

    if (!q.exec()) {
        emit databaseError("Erreur closeOrphanTests : " + q.lastError().text());
        return;
    }

    int count = q.numRowsAffected();
    if (count > 0)
        qDebug() << "[DB] " << count << " test(s) orphelin(s) cloture(s) au demarrage.";

    emit testListChanged();
}

void DatabaseManager::onNewPacket(const DataRecord& record)
{
    if (!m_db.isOpen()) return;

    m_buffer.push(record);

    // Flush dès que le buffer atteint 500 paquets.
    // Le flush automatique (QTimer 2s) couvre déjà les situations de très faibles débits.
    if (m_buffer.isFull()) {
        flushBuffer();
    }
}

void DatabaseManager::flushBuffer()
{
    if (!m_db.isOpen() || m_buffer.size() == 0) return;

    flushToDatabase(m_buffer.records());
    m_buffer.clear();
}

void DatabaseManager::flushToDatabase(const QVector<DataRecord>& records)
{
    // Une seule transaction pour les N insertions :
    // ~50x plus rapide que N transactions individuelles.
    m_db.transaction();

    QSqlQuery q(m_db);

    // On prépare la requête une seule fois (bindValue ensuite pour chaque ligne).
    q.prepare(R"(
        INSERT INTO sensor_data (
            test_id, is_test, timestamp, packet_id, teensy_millis,
            PS11, PS12, PS21, PS22, PS23, PS31, PS41, PS42, PS51,
            PS61, PS62, PS63, PS64, PS71, PS81,
            TS11, TS12, TS41, TS42, TS61, TS62,
            FM11, FM21, FM61,
            LC, ref5V, glowplug,
            valvesState, actLPos, actRPos, actLOK, actROK,
            state, test_step, test_cooling
        ) VALUES (
            :test_id, :is_test, :timestamp, :packet_id, :teensy_millis,
            :PS11, :PS12, :PS21, :PS22, :PS23, :PS31, :PS41, :PS42, :PS51,
            :PS61, :PS62, :PS63, :PS64, :PS71, :PS81,
            :TS11, :TS12, :TS41, :TS42, :TS61, :TS62,
            :FM11, :FM21, :FM61,
            :LC, :ref5V, :glowplug,
            :valvesState, :actLPos, :actRPos, :actLOK, :actROK,
            :state, :test_step, :test_cooling
        )
    )");

    for (const DataRecord& r : records) {
        // test_id peut être NULL (hors test) → on passe QVariant() dans ce cas.
        q.bindValue(":test_id",       r.test_id >= 0 ? QVariant(r.test_id) : QVariant(QMetaType(QMetaType::Int)));
        q.bindValue(":is_test",       r.is_test);
        q.bindValue(":timestamp",     r.timestamp.toString(Qt::ISODateWithMs));
        q.bindValue(":packet_id",     r.packet_id);
        q.bindValue(":teensy_millis", r.teensy_millis);

        q.bindValue(":PS11", r.PS11); q.bindValue(":PS12", r.PS12);
        q.bindValue(":PS21", r.PS21); q.bindValue(":PS22", r.PS22);
        q.bindValue(":PS23", r.PS23); q.bindValue(":PS31", r.PS31);
        q.bindValue(":PS41", r.PS41); q.bindValue(":PS42", r.PS42);
        q.bindValue(":PS51", r.PS51);
        q.bindValue(":PS61", r.PS61); q.bindValue(":PS62", r.PS62);
        q.bindValue(":PS63", r.PS63); q.bindValue(":PS64", r.PS64);
        q.bindValue(":PS71", r.PS71); q.bindValue(":PS81", r.PS81);

        q.bindValue(":TS11", r.TS11); q.bindValue(":TS12", r.TS12);
        q.bindValue(":TS41", r.TS41); q.bindValue(":TS42", r.TS42);
        q.bindValue(":TS61", r.TS61); q.bindValue(":TS62", r.TS62);

        q.bindValue(":FM11", r.FM11);
        q.bindValue(":FM21", r.FM21);
        q.bindValue(":FM61", r.FM61);

        q.bindValue(":LC",      r.LC);
        q.bindValue(":ref5V",   r.ref5V);
        q.bindValue(":glowplug", r.glowplug);

        q.bindValue(":valvesState", r.valvesState);
        q.bindValue(":actLPos", r.actLPos); q.bindValue(":actRPos", r.actRPos);
        q.bindValue(":actLOK",  r.actLOK);  q.bindValue(":actROK",  r.actROK);

        q.bindValue(":state",        r.state);
        q.bindValue(":test_step",    r.test_step);
        q.bindValue(":test_cooling", r.test_cooling ? 1 : 0);

        if (!q.exec()) {
            emit databaseError("Erreur insertion : " + q.lastError().text());
        }
    }

    m_db.commit();
}

int DatabaseManager::startTest(const QString& sequenceFile)
{
    if (!m_db.isOpen()) return -1;

    QDateTime now  = QDateTime::currentDateTime();
    // Nom automatique basé sur la date, lisible et unique.
    QString   name = "Test_" + now.toString("yyyy-MM-dd_HH:mm:ss");

    QSqlQuery q(m_db);
    //prepare pour insertion plus clean
    q.prepare(R"(
        INSERT INTO tests (name, started_at, sequence_file)
        VALUES (:name, :started_at, :sequence_file)
    )");
    q.bindValue(":name",          name);
    q.bindValue(":started_at",    now.toString(Qt::ISODateWithMs));
    q.bindValue(":sequence_file", sequenceFile.isEmpty() ? QVariant() : sequenceFile);

    if (!q.exec()) { //Check si test créé correctement
        emit databaseError("Erreur création test : " + q.lastError().text());
        return -1;
    }

    m_currentTestId = q.lastInsertId().toInt();
    qDebug() << "[DB] Test démarré :" << name << "(id=" << m_currentTestId << ")";
    emit testListChanged();
    return m_currentTestId;
}

void DatabaseManager::endTest(int testId)
{
    if (!m_db.isOpen() || testId < 0) return;

    // On flush d'abord pour ne pas perdre les dernières mesures du test.
    flushBuffer();

    QSqlQuery q(m_db);
    q.prepare("UPDATE tests SET ended_at = :ended_at WHERE id = :id");
    q.bindValue(":ended_at", QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
    q.bindValue(":id",       testId);

    if (!q.exec())
        emit databaseError("Erreur fin de test : " + q.lastError().text());

    m_currentTestId = -1; // Retour en mode idle
    qDebug() << "[DB] Test terminé (id=" << testId << ")";
    emit testListChanged();
}

void DatabaseManager::clearIdleData() //idle = données hors test (is_test = 0)
{
    if (!m_db.isOpen()) return;

    // Flush d'abord pour traiter ce qui est encore en buffer.
    flushBuffer();

    QSqlQuery q(m_db);
    // is_test = 1 (test normal) et is_test = 2 (urgence) sont conservés.
    if (!q.exec("DELETE FROM sensor_data WHERE is_test = 0")) {
        emit databaseError("Erreur purge idle : " + q.lastError().text());
        return;
    }

    // VACUUM récupère les cases vides libérées par les suppressions.
    q.exec("VACUUM");
    qDebug() << "[DB] Données idle supprimées.";
}

QVector<DatabaseManager::TestInfo> DatabaseManager::getTestList()
{
    QVector<TestInfo> list;
    if (!m_db.isOpen()) return list;

    QSqlQuery q(m_db);
    // Du plus récent au plus ancien car c'est plus pratique pour l'utilisateur (le test le plus récent est forcément en tête de liste).
    q.exec("SELECT id, name, started_at, ended_at, sequence_file FROM tests ORDER BY id DESC");

    while (q.next()) {
        TestInfo t;
        t.id           = q.value(0).toInt();
        t.name         = q.value(1).toString();
        t.startedAt    = q.value(2).toString();
        t.endedAt      = q.value(3).toString();    // vide si NULL
        t.sequenceFile = q.value(4).toString();
        list.append(t);
    }

    return list;
}

// ============================================================
//  Export CSV
// ============================================================

// Liste fixe de tous les noms de colonnes capteurs dans l'ordre de la DB.
QStringList DatabaseManager::allSensorColumns()
{
    return {
        "PS11","PS12","PS21","PS22","PS23","PS31",
        "PS41","PS42","PS51",
        "PS61","PS62","PS63","PS64","PS71","PS81",
        "TS11","TS12","TS41","TS42","TS61","TS62",
        "FM11","FM21","FM61",
        "LC","ref5V","glowplug",
        "valvesState","actLPos","actRPos","actLOK","actROK",
        "state","test_step","test_cooling"
    };
}

// Facteurs d'ordre de grandeurs indiqués à l'export (identiques à l'ancien CSV).
// Retourne 1.0 si aucun facteur n'est défini pour cette colonne.
static double scaleFactorFor(const QString& col)
{
    if (col.startsWith("PS"))   return 1.0 / 1000.0;   // mbar → bar
    if (col.startsWith("TS"))   return 1.0 / 10.0;     // ×10 → °C
    if (col == "ref5V")         return 1.0 / 10000.0;  // ×0.1mV → V
    if (col == "glowplug")      return 1.0 / 1000.0;   // mA → A
    return 1.0; // FM, LC, états → brut
}

// Construit la clause WHERE pour filtrer par testId et/ou plage temporelle.
QString DatabaseManager::buildWhereClause(int              testId, 
                                          const QDateTime& from,
                                          const QDateTime& to)
{
    QStringList conditions;
    if (testId >= 0)
        conditions << QString("test_id = %1").arg(testId);

    if (from.isValid())
        conditions << QString("timestamp >= '%1'").arg(from.toString(Qt::ISODateWithMs));

    if (to.isValid())
        conditions << QString("timestamp <= '%1'").arg(to.toString(Qt::ISODateWithMs));

    return conditions.isEmpty() ? "" : " WHERE " + conditions.join(" AND ");
}

// Retourne le nombre de lignes correspondant aux critères (sans ouvrir de fichier).
int DatabaseManager::countData(int testId, const QDateTime& from, const QDateTime& to)
{
    if (!m_db.isOpen()) return 0;
    flushBuffer();
    QSqlQuery q(m_db);
    if (!q.exec("SELECT COUNT(*) FROM sensor_data" + buildWhereClause(testId, from, to)))
        return 0;
    return q.next() ? q.value(0).toInt() : 0;
}

// Export CSV selon les critères donnés. Applique les facteurs d'ordre de grandeur aux valeurs capteurs.
bool DatabaseManager::exportToCsv(const QString&     outputPath,
                                  const QStringList& sensors,
                                  int                testId,
                                  const QDateTime&   from,
                                  const QDateTime&   to)
{
    if (!m_db.isOpen()) return false;

    // Flush pour inclure les données encore en buffer.
    flushBuffer();

    // Détermine les colonnes à exporter.
    // Si la liste est vide, on prend tout.
    QStringList allCols  = allSensorColumns();
    QStringList exportCols;

    if (sensors.isEmpty()) {
        exportCols = allCols;
    } else {
        // SI sensors n'est pas vide on inclut seulement les capteurs demandés par l'utilisateur
        for (const QString& col : allCols) {
            if (sensors.contains(col, Qt::CaseInsensitive))
                exportCols << col;
        }
    }

    // Colonnes fixes toujours présentes en tête devant les colonnes capteurs (pour faciliter l'analyse)
    QString select = "timestamp, packet_id, teensy_millis, is_test, test_id, "
                   + exportCols.join(", ");

    QString sql = "SELECT " + select + " FROM sensor_data"
                + buildWhereClause(testId, from, to)
                + " ORDER BY timestamp ASC";

    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        emit databaseError("Erreur export CSV : " + q.lastError().text());
        return false;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit databaseError("Impossible d'écrire le fichier : " + outputPath);
        return false;
    }

    QTextStream out(&file);

    // ---- En-tête CSV ----
    // Les 5 colonnes fixes + colonnes capteurs avec leur unité.
    QStringList headerParts = {"timestamp", "packet_id", "teensy_ms", "is_test", "test_id"};
    for (const QString& col : exportCols)
        headerParts << col;
    out << headerParts.join(",") << "\n";

    // ---- Lignes de données ----
    // Colonnes 0-4 : timestamp, packet_id, teensy_millis, is_test, test_id
    // Colonnes 5+  : valeurs capteurs, avec facteur d'ordre de grandeur appliqué.
    while (q.next()) {
        QStringList parts;

        // Colonnes fixes (indices 0 à 4)
        parts << q.value(0).toString();    // timestamp
        parts << q.value(1).toString();    // packet_id
        parts << q.value(2).toString();    // teensy_millis
        parts << q.value(3).toString();    // is_test
        parts << q.value(4).toString();    // test_id

        // Colonnes capteurs (indices 5 et suivants)
        for (int i = 0; i < exportCols.size(); ++i) {
            double rawValue = q.value(5 + i).toDouble();
            double scaled   = rawValue * scaleFactorFor(exportCols[i]);

            // Évite la notation scientifique dans le CSV et donne une précision de 6 chiffres après la virgule.
            parts << QString::number(scaled, 'f', 6);
        }

        out << parts.join(",") << "\n"; // rajoute la ligne au fichier CSV
    }

    file.close();
    qDebug() << "[DB] CSV exporté :" << outputPath; // Log pour confirmer que l'export a réussi
    return true;
}
                                
