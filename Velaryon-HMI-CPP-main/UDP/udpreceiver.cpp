#include "UDP/udpreceiver.h"
#include "valve/valve.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include "UDP/global_variable.h"
#include "tools/loghelper.h"
#include <QThread>
#include <QTimer>
#include <QQueue>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// Global flags and variables for test status tracking
bool ack = false;
bool launch_test = false;
int countdown = 0;
bool end_test = false;

// Global log file paths with timestamp
QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

QString basePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                   + "/Velaryon-HMI/output_data";

QString testFolderName = QString("test_%1").arg(timestamp);
QDir testDir(QDir(basePath).filePath(testFolderName));

QString filepath_log    = testDir.filePath(QString("log%1.txt").arg(timestamp));
QString filepath_csv    = testDir.filePath(QString("data_%1.csv").arg(timestamp));
QString filepath_csv_HF = testDir.filePath(QString("data_HF_%1.csv").arg(timestamp));

// Sensor data variables
uint32_t ID = 0, timing = 0, valvesState = 0;
int32_t PS31 = 0, PS51 = 0, LC = 0;
uint16_t FM11 = 0, FM21 = 0, FM61 = 0, ref5V = 0, glowplug = 0;
int16_t PS11 = 0, PS12 = 0, PS21 = 0, PS22 = 0, PS23 = 0, PS41 = 0, PS42 = 0, PS61 = 0, PS62 = 0, PS63 = 0, PS64 = 0, PS71 = 0, PS81 = 0;
int16_t TS11 = 0, TS12 = 0, TS41 = 0, TS42 = 0, TS61 = 0, TS62 = 0;
uint8_t actLPos = 0, actRPos = 0, actLOK = 0, actROK = 0, state = 0, test_step = 0;

// Constructor: Set up UDP socket and periodic timers
UdpReceiver::UdpReceiver(QObject *parent) : QObject(parent) {
    udpSocket = new QUdpSocket(this);
    QHostAddress address(Config::getComputerIpAddress());
    quint16 port = Config::getComputerPort();

    if (!udpSocket->bind(address, port)) {
        qDebug() << "Failed to bind UDP socket to" << address.toString() << ":" << port;
        return;
    }

    // Connect to incoming data
    connect(udpSocket, &QUdpSocket::readyRead, this, &UdpReceiver::processPendingDatagrams);

    // Timer to flush buffered CSV data to file
    flushTimer = new QTimer(this);
    connect(flushTimer, &QTimer::timeout, this, &UdpReceiver::flushAllCsvBuffers);
    flushTimer->start(50);  // Flush every 50 ms

    // Timer to process latest sensor data
    processTimer = new QTimer(this);
    connect(processTimer, &QTimer::timeout, this, &UdpReceiver::processQueuedData);
    processTimer->start(50); // Process every 50ms
}

// Destructor: cleanup
UdpReceiver::~UdpReceiver() {
    delete udpSocket;
}

// Connecte le DatabaseManager &#8212; appelé depuis main.cpp
void UdpReceiver::setDatabaseManager(DatabaseManager* mgr) {
    m_dbManager = mgr;
}

// Emit the latest available sensor values
void UdpReceiver::processQueuedData() {
    QVector<QVariant> dataCopy;

    {
        // Lock the mutex to ensure thread-safe access to shared data
        QMutexLocker locker1(&queueMutex);

        // If there's no new data to process, exit early
        if (latestData.isEmpty())
            return;

        // Copy the latest received data into the local variable
        dataCopy = latestData;

        // Clear the shared data so we know it has been processed
        latestData.clear();
    } // Mutex is automatically released here when locker1 goes out of scope

    // Emit the copied data to the connected slots
    // This is done after releasing the lock to avoid blocking the receiver thread
    emit sensor_value(dataCopy);
}

// Flush CSV data from buffers to file
void UdpReceiver::flushAllCsvBuffers() {
    // qDebug() << "flush ";
    flushCsvBuffer(filepath_csv, csvBuffer, csvMutex);
    // qDebug() << "Csv Buffer : " << csvBuffer;
    flushCsvBuffer(filepath_csv_HF, csvBuffer2, csvMutex2);
}

// Add a line of data to a CSV
void UdpReceiver::logVectorToCSV(const QVector<QVariant> &dataVector, QStringList &buffer, QMutex &mutex) {
    QString line;
    QTextStream out(&line);

    // Transform array to one single string
    for (int i = 0; i < dataVector.size(); ++i) {
        out << dataVector[i].toString();
        // qDebug() <<" Vector size : " << dataVector.size();
        if (i < dataVector.size() - 1) out << ",";
    }

    // Send the string to the mutex
    QMutexLocker locker(&mutex);
    // qDebug() << "Line : " << line;
    buffer.append(line);
    // qDebug()<< "Taille du buffer : " << buffer.size();
    // qDebug()<< "Buffer : " << buffer[0];
}

// Flush buffer to a CSV file
void UdpReceiver::flushCsvBuffer(const QString &filepath, QStringList &buffer, QMutex &mutex) {
    // When timer's up, check the mutex

    QMutexLocker locker(&mutex);
    if (buffer.isEmpty()) {/*qDebug()<< "in"*/;return;}

    // Look for filepath
    QFileInfo fileInfo(filepath);
    QDir dir = fileInfo.absoluteDir();

    // if (!dir.exists()) {
    //     qDebug() << "Directory does not exist. Creating:" << dir.absolutePath();
    //     dir.mkpath(".");    // Create the full directory path if it doesn't exist

    // }

    if (!testDir.exists()) {
        QDir().mkpath(testDir.absolutePath());
    }

    // Create a file
    QFile file(filepath);
    // qDebug() << 'Filepath : ' << filepath ;
    bool writeHeader = !file.exists();

    //Open file in append mode and write only
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);

        // Writte header if the file is empty
        if (writeHeader) {
            out << "ID,time,PS11,PS12,PS21,PS22,PS23,PS31,PS41,PS42,PS51,PS61,PS62,PS63,PS64,PS71,PS81,"
                   "TS11,TS12,TS41,TS42,TS61,TS62,FM11,FM21,FM61,LC,ref5V,glowplug,valvesState,"
                   "actLPos,actRPos,actLOK,actROK,state,test_step\n";
        }

        // Write data in csv
        for (const QString &line : buffer) {
            out << line << "\n";
            // qDebug() << "Line : " << line;
        }

        // Clear buffer and close file
        buffer.clear();
        file.close();
    } else {
        qDebug() << "Failed to open CSV file for flushing.";
    }
}

// Main function to process all incoming UDP datagrams
void UdpReceiver::processPendingDatagrams() {
    while (udpSocket->hasPendingDatagrams()) {

        // Register the incoming packet as a byte array
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());

        // Sensor data
        if (static_cast<unsigned char>(datagram[0]) == 0xFF &&
            static_cast<unsigned char>(datagram[1]) == 0xFF &&
            static_cast<unsigned char>(datagram[2]) == 0xFF &&
            static_cast<unsigned char>(datagram[3]) == 0xFF) {


            // Remove the header
            QByteArray dataWithoutHeader = datagram.mid(4);
            QDataStream stream(dataWithoutHeader);
            stream.setByteOrder(QDataStream::LittleEndian);


            // qDebug() << "--------------------------------------------";
            // Automatically attribute the bytes to the variables
            //        0        1
            stream >> ID >> timing;
            // qDebug() << "[RX] ID:" << ID << "timing:" << timing;
            //          2       3      4        5      6        7      8        9      10      11      12      13      14       15      16
            stream >> PS11 >> PS12 >> PS21 >> PS22 >> PS23 >> PS31 >> PS41 >> PS42 >> PS51 >> PS61 >> PS62 >> PS63 >> PS64 >> PS71 >> PS81;
            // qDebug() << "[RX] PS:"
            //          << "PS11" << PS11 << "PS12" << PS12
            //          << "PS21" << PS21 << "PS22" << PS22 << "PS23" << PS23
            //          << "PS31" << PS31
            //          << "PS41" << PS41 << "PS42" << PS42
            //          << "PS51" << PS51
            //          << "PS61" << PS61 << "PS62" << PS62 << "PS63" << PS63 << "PS64" << PS64
            //          << "PS71" << PS71 << "PS81" << PS81;
            //         17      18      19     20       21      22
            stream >> TS11 >> TS12 >> TS41 >> TS42 >> TS61 >> TS62;
            // qDebug() << "[RX] TS:"
            //          << "TS11" << TS11 << "TS12" << TS12
            //          << "TS41" << TS41 << "TS42" << TS42
            //          << "TS61" << TS61 << "TS62" << TS62;
            //         23      24      25
            stream >> FM11 >> FM21 >> FM61;
            // qDebug() << "[RX] FM:" << "FM11" << FM11 << "FM21" << FM21 << "FM61" << FM61;
            //        26      27       28         29
            stream >> LC >> ref5V >> glowplug >> valvesState;
            // qDebug() << "[RX] Misc:"
            //          << "LC" << LC
            //          << "ref5V" << ref5V
            //          << "glowplug" << glowplug
            //          << "valvesState" << valvesState;
            //          30        31          32       33
            stream >> actLPos >> actRPos >> actLOK >> actROK;
            // qDebug() << "[RX] Actuators:"
            //          << "actLPos" << actLPos
            //          << "actRPos" << actRPos
            //          << "actLOK" << actLOK
            //          << "actROK" << actROK;
            //          34        35
            stream >> state >> test_step;
            // qDebug() << "[RX] State:" << "state" << state << "test_step" << test_step;

            // ---- Synchronisation temps réel avec détection de reboot ----
            if (!m_timeSynced) {
                m_T0_pc            = QDateTime::currentDateTime();
                m_T0_teensy        = timing;
                m_lastTeensyMillis = timing;
                m_millisOffset     = 0;
                m_timeSynced       = true;
            } else {
                // Détection de reboot/rollover : si le millis() actuel est inférieur
                // au précédent, la Teensy a redémarré. On accumule l'offset.
                if (timing < m_lastTeensyMillis) {
                    m_millisOffset += static_cast<qint64>(m_lastTeensyMillis) - static_cast<qint64>(m_T0_teensy) + static_cast<qint64>(timing);
                    m_T0_teensy = timing;  // Nouveau point de référence
                }
                m_lastTeensyMillis = timing;
            }

            // Calcul du timestamp réel avec prise en compte de l'offset cumulé;
            qint64 elapsedMs = m_millisOffset + static_cast<qint64>(timing) - static_cast<qint64>(m_T0_teensy);
            QDateTime realTimestamp = m_T0_pc.addMSecs(elapsedMs);
            // ----------------------------------------------

            //----------------------------------
            // Convert the variable as binary
            QString binary = QString::number(valvesState, 2).rightJustified(20, '0');
            QString reversedBinary = "";
            int binaryLength = binary.length();  // The length of the binary string

            // Update all the valve states using the binary (SSOT)

            for (auto& valve : valveMap) {
                const QString name = valve.name;                // adapte si ton champ s'appelle autrement
                const int id = valve_Name_ID.value(name, -1);   // -1 si inconnu

                if (id >= 0 && id < binaryLength) {
                    const QChar bit = binary[binaryLength - 1 - id];

                    if (bit == '1') {
                        // qDebug() << "Vanne : "<< valve.name << "id " << id << "Open" ;
                        valve.statusLabel->setText("Open");
                        valve.statusLabel->setStyleSheet("QLabel { color: green; }");
                    } else {
                        // qDebug() << "Vanne : "<< valve.name << "id " << id << "Close" ;
                        valve.statusLabel->setText("Close");
                        valve.statusLabel->setStyleSheet("QLabel { color: red; }");
                    }
                } else {
                    // Si la vanne n'est pas mappée ou hors plage, optionnel :
                    valve.statusLabel->setText("N/A");
                    valve.statusLabel->setStyleSheet("QLabel { color: gray; }");
                }
            }
            //----------------------------------

            // ---- Envoi vers la base de données ----
            if (m_dbManager) {
                DataRecord rec;
                rec.timestamp     = realTimestamp;
                rec.is_test       = static_cast<int>(state);
                rec.test_id       = m_dbManager->currentTestId();
                rec.packet_id     = ID;
                rec.teensy_millis = timing;

                rec.PS11 = PS11; rec.PS12 = PS12;
                rec.PS21 = PS21; rec.PS22 = PS22; rec.PS23 = PS23;
                rec.PS31 = PS31;
                rec.PS41 = PS41; rec.PS42 = PS42;
                rec.PS51 = PS51;
                rec.PS61 = PS61; rec.PS62 = PS62; rec.PS63 = PS63; rec.PS64 = PS64;
                rec.PS71 = PS71; rec.PS81 = PS81;

                rec.TS11 = TS11; rec.TS12 = TS12;
                rec.TS41 = TS41; rec.TS42 = TS42;
                rec.TS61 = TS61; rec.TS62 = TS62;

                rec.FM11 = FM11; rec.FM21 = FM21; rec.FM61 = FM61;

                rec.LC       = LC;
                rec.ref5V    = ref5V;
                rec.glowplug = glowplug;

                rec.valvesState  = valvesState;
                rec.actLPos = actLPos; rec.actRPos = actRPos;
                rec.actLOK  = actLOK;  rec.actROK  = actROK;
                rec.state        = state;
                rec.test_step    = test_step;
                rec.test_cooling = false;

                m_dbManager->onNewPacket(rec);
            }
            // -------------------------------------------------

            // Append the values to an array
            QVector<QVariant> data;
            data.append(ID);
            data.append(timing / 1000.0);
            data.append(PS11 / 1000.0); data.append(PS12 / 1000.0); data.append(PS21 / 1000.0); data.append(PS22 / 1000.0);
            data.append(PS23 / 1000.0); data.append(PS31 / 1000.0); data.append(PS41 / 1000.0); data.append(PS42 / 1000.0);
            data.append(PS51 / 1000.0); data.append(PS61 / 1000.0); data.append(PS62 / 1000.0); data.append(PS63 / 1000.0);
            data.append(PS64 / 1000.0); data.append(PS71 / 1000.0); data.append(PS81 / 1000.0);
            data.append(TS11 / 10.0); data.append(TS12 / 10.0); data.append(TS41 / 10.0); data.append(TS42 / 10.0);
            data.append(TS61 / 10.0); data.append(TS62 / 10.0);
            data.append(FM11); data.append(FM21); data.append(FM61);
            data.append(LC); data.append(ref5V / 10000.0); data.append(glowplug / 1000.0); data.append(valvesState);
            data.append(actLPos); data.append(actRPos); data.append(actLOK); data.append(actROK);
            data.append(state); data.append(test_step);

            // Lock mutex & log data in "main" csv
            QMutexLocker locker1(&queueMutex);
            latestData = data;
            logVectorToCSV(latestData, csvBuffer, csvMutex);

            // Log in HF csv if time difference between two packets exceeds threshold
            quint32 timeDiff = timing - previousTiming;
            if (timeDiff < 20) {    // Threshold for HF logging (f>50Hz)
                logVectorToCSV(latestData, csvBuffer2, csvMutex2);
            }

            previousTiming = timing;
        }

        // ON/OFF glowplug
        else if (static_cast<unsigned char>(datagram[0]) == 0xFF &&
                 static_cast<unsigned char>(datagram[1]) == 0xFF &&
                 static_cast<unsigned char>(datagram[2]) == 0xAA &&
                 static_cast<unsigned char>(datagram[3]) == 0xAA) {
            ack = true;
        }

        // Ack valve && BB
        else if (static_cast<unsigned char>(datagram[0]) == 0xEE &&
                 static_cast<unsigned char>(datagram[1]) == 0xEE &&
                 static_cast<unsigned char>(datagram[2]) == 0xFF &&
                 static_cast<unsigned char>(datagram[3]) == 0xFF) {
            ack = true;
        }

        // Ack test (1st check)
        else if (static_cast<unsigned char>(datagram[0]) == 0xEE &&
                 static_cast<unsigned char>(datagram[1]) == 0xEE &&
                 static_cast<unsigned char>(datagram[2]) == 0xAA &&
                 static_cast<unsigned char>(datagram[3]) == 0xAA) {
            ack = true;
            logMessage(globalLogTerminal, "Sequence received");
        }

        // Confirm test (2nd check)
        else if (static_cast<unsigned char>(datagram[0]) == 0xBB &&
                 static_cast<unsigned char>(datagram[1]) == 0xBB &&
                 static_cast<unsigned char>(datagram[2]) == 0xBB &&
                 static_cast<unsigned char>(datagram[3]) == 0xBB) {
            launch_test = true;
        }

        // Countdown
        else if (static_cast<unsigned char>(datagram[0]) == 0xAB &&
                 static_cast<unsigned char>(datagram[1]) == 0xAB &&
                 static_cast<unsigned char>(datagram[2]) == 0xAB &&
                 static_cast<unsigned char>(datagram[3]) == 0xAB) {
            QByteArray dataWithoutHeader = datagram.mid(4);
            QDataStream stream(dataWithoutHeader);
            stream.setByteOrder(QDataStream::BigEndian);
            stream >> countdown;
        }

        // Warning
        else if (static_cast<unsigned char>(datagram[0]) == 0xDD &&
                 static_cast<unsigned char>(datagram[1]) == 0xDD &&
                 static_cast<unsigned char>(datagram[2]) == 0xDD &&
                 static_cast<unsigned char>(datagram[3]) == 0xDD) {
            QByteArray messageData = datagram.mid(4);
            QString message = QString::fromUtf8(messageData);
            logMessage(globalLogTerminal, message);
        }

        // Emergency error
        else if (static_cast<unsigned char>(datagram[0]) == 0xCC &&
                 static_cast<unsigned char>(datagram[1]) == 0xCC &&
                 static_cast<unsigned char>(datagram[2]) == 0xCC &&
                 static_cast<unsigned char>(datagram[3]) == 0xCC) {
            QByteArray messageData = datagram.mid(4);
            QString message = QString::fromUtf8(messageData);
            logMessage(globalLogTerminal, message);
        }

        else if (static_cast<unsigned char>(datagram[0]) == 0xAB &&
                 static_cast<unsigned char>(datagram[1]) == 0xCD &&
                 static_cast<unsigned char>(datagram[2]) == 0xAB &&
                 static_cast<unsigned char>(datagram[3]) == 0xCD) {
            end_test = true;
        }

        // Unknown command
        else {
            qDebug() << "Unknown command received" << datagram;
        }
    }
}
