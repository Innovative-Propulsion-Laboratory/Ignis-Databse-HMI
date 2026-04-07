#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include "UDP/config.h"
#include "UDP/udpsender.h"
#include <QFile>
#include <QElapsedTimer>
#include <QMutex>
#include <QTimer>
#include <QQueue>
#include <QDateTime>
#include "Database/database_manager.h"

// UdpReceiver listens for incoming UDP messages, processes them,
// logs data to CSV, and can send replies via UdpSender.
class UdpReceiver : public QObject {
    Q_OBJECT
public:
    explicit UdpReceiver(QObject *parent = nullptr);
    void logVectorToCSV(const QVector<QVariant> &dataVector, QStringList &buffer, QMutex &mutex);
    void flushCsvBuffer(const QString &filepath, QStringList &buffer, QMutex &mutex);
    void processQueuedData();
    void flushAllCsvBuffers();
    void setDatabaseManager(DatabaseManager* mgr);
    ~UdpReceiver();

private slots:
    void processPendingDatagrams();

signals:
    void valveStatusChanged(int valveId, bool open);        // Emitted when a valve's state changes
    void sensor_value(const QVector<QVariant> &values);     // Emitted when new sensor values are received

private:
    QUdpSocket *udpSocket;              // UDP socket for receiving datagrams
    UdpSender sender;                   // Embedded UdpSender used for sending responses or ACKs

    QVector<QVariant> latestData;       // Stores the most recent received data values
    QMutex queueMutex;                  // Protects access to the queued data
    QTimer *processTimer = nullptr;     // Timer for periodically processing the data queue

    QStringList csvBuffer;              // CSV buffer for logging incoming data (all)
    QMutex csvMutex;
    QStringList csvBuffer2;             // CSV buffer for logging incoming data (HF)
    QMutex csvMutex2;

    quint32 previousTiming = 0;         // Keeps track of the timestamp of the last received message
    QTimer *flushTimer = nullptr;       // Timer to periodically flush CSV buffers to disk

    // Real-time synchronisation
    bool      m_timeSynced  = false;    // True after the first UDP packet is received
    QDateTime m_T0_pc;                  // PC wall-clock time of the first packet
    uint32_t  m_T0_teensy   = 0;        // Teensy millis() value of the first packet

    // Anti-rollback millis()
    uint32_t  m_lastTeensyMillis = 0;   // Dernier millis() recu
    qint64    m_millisOffset     = 0;   // Offset cumule ajoute apres chaque reboot

    // Database pointer
    DatabaseManager* m_dbManager = nullptr;  // "Non-owning" pointer, gere par main.cpp

};

#endif // UDPRECEIVER_H
