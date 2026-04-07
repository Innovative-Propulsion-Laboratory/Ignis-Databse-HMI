/*****************************************************************************************
**                                                                                      **
**   UdpSender handles UDP message sending for various control commands such as         **
**   valve operations, TVC tests, BB control, and pressure settings. Uses               **
**   acknowledgment mechanism to ensure reliability. Integrated with logging system.    **
**                                                                                      **
*****************************************************************************************/
#include "UDP/udpsender.h"
#include "Glowplug/glowplug.h"
#include "UDP/config.h"
#include "UDP/global_variable.h"
#include "tools/loghelper.h"
#include <QElapsedTimer>
#include <QCoreApplication>

// Constructor: initializes destination IP and port from config
UdpSender::UdpSender(QObject *parent)
    : QObject(parent),
    destinationIp(Config::getUC_IpAddress()),
    destinationPort(Config::getUC_Port()) {
    // UDP socket is initialized and ready to send messages
}

// Sends a raw QByteArray message over UDP to the configured IP and port
void UdpSender::sendMessage(const QByteArray &data) {
    udpSocket.writeDatagram(data, destinationIp, destinationPort);
}

// Sends a command to open or close a valve and waits for acknowledgment
void UdpSender::sendValve(Valve valve, bool open) {
    QByteArray data;

    // Header for valve control: 4 bytes of 0xFF
    for (int j = 0; j < 4; j++) {
        data.append(static_cast<char>(0xFF));
    }

    data.append(static_cast<char>(valve.id & 0xFF));    // Append valve ID
    data.append(static_cast<char>(open ? 1 : 0));       // Append action: 1 = open, 0 = close

    // Reset acknowledgement flag and send the data
    ack=false;
    sendMessage(data);

    // Wait up to 1 second for acknowledgment
    QElapsedTimer timer;
    timer.start();
    while (!ack && timer.elapsed() < 1000) {
        QCoreApplication::processEvents();      // Allows Qt to process UDP events
    }

    // qDebug() << "Valve ID : " << valve.id;

    // Improvement: Instead of storing valve names in a separate array,
    // the valve's name could be accessed directly from valve.label->text()
    // Array of valve names

    QString text = valve.name;

    // Update label and log message only if ACK was received
    // Note: Updating the label is a redundancy since UdpReceiver already updates states
    if (ack){
        if (open) {
            valve.statusLabel->setText("Open");
            valve.statusLabel->setStyleSheet("QLabel { color: green; }");
            logMessage(globalLogTerminal, text + " opened");
        } else {
            valve.statusLabel->setText("Close");
            valve.statusLabel->setStyleSheet("QLabel { color: red; }");
            logMessage(globalLogTerminal, text + " closed");
        }
    }

    ack=false; // Reset for next command
}

void UdpSender::sendGlowplug(Glowplug glowplug, bool on) {
    QByteArray data;
    // Header for glowpug control: 2 bytes of 0xFF, 2 bytes of 0xAA
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xFF));
    }
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xAA));
    }
    data.append(static_cast<char>(glowplug.id));
    data.append(static_cast<char>(on ? 1 : 0));

    // Reset acknowledgement flag and send the data
    ack=false;
    sendMessage(data);

    // Wait up to 1 second for acknowledgment
    QElapsedTimer timer;
    timer.start();
    while (!ack && timer.elapsed() < 1000) {
        QCoreApplication::processEvents();      // Allows Qt to process UDP events
    }

    QString text = "Glowplug";

    if (ack){
        if (on) {
            glowplug.statusLabel->setText("On");
            glowplug.statusLabel->setStyleSheet("QLabel { color: green; }");
            logMessage(globalLogTerminal, text + " opened");
        } else {
            glowplug.statusLabel->setText("Off");
            glowplug.statusLabel->setStyleSheet("QLabel { color: red; }");
            logMessage(globalLogTerminal, text + " closed");
        }
    }

    ack=false; // Reset for next command
}

// Sends a test signal for TVC (Thrust Vector Control) movement with a shape ID
// Note: Work might be required since it was not used for hotfires
void UdpSender::sendTVCtest(int shape) {
    QByteArray data;

    // Header for TVC test: 2 bytes of 0xEE and 2 bytes of 0xDD
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xEE));
    }
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xDD));
    }

    // Append shape ID
    data.append(static_cast<char>(shape));

    sendMessage(data);
}

// Sends a command to enable or disable a BB (bang-bang) tank system
bool UdpSender::sendBBcontrol(int tank, int activate) {
    QByteArray data;

    // Header: 2 bytes of 0xFF + 2 bytes of 0xDD
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xFF));
    }
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xDD));
    }

    // Append tank number and activation flag
    data.append(static_cast<char>(tank));
    data.append(static_cast<char>(activate));

    // Reset acknowledgement flag and send the data
    ack=false;
    sendMessage(data);

    // Wait up to 1 second for acknowledgment
    QElapsedTimer timer;
    timer.start();
    while (!ack && timer.elapsed() < 1000) {
        QCoreApplication::processEvents();      // Allows Qt to process UDP events
    }

    if (ack)
    {
        ack=false;

        // Write a log to inform BB state
        QString text,text2 = " enabled";
        if (tank == 1){text = "LOX ";}
        else if (tank == 2){text = "ETH ";}
        else if (tank == 6){text = "H2O ";}
        if (activate == 0){text2 = " disabled";}

        logMessage(globalLogTerminal, text+"BB"+text2);
        return true;
    }

    ack=false;
    return false;
}

// Sends a command to set the target pressure for a specific tank
void UdpSender::sendsetpressure(int tank, float pressure) {

    QByteArray data;

    // Header: 2 bytes of 0xFF + 2 bytes of 0xEE
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xFF));
    }
    for (int j = 0; j < 2; j++) {
        data.append(static_cast<char>(0xEE));
    }

    // Append tank ID
    data.append(static_cast<char>(tank));

    // Append pressure value (in mbar) as two bytes (big-endian)
    for (int i = 1; i >= 0; i--) {
        data.append(static_cast<char>((int(1000*pressure) >> (i * 8)) & 0xFF));
    }

    sendMessage(data);

    // Write a log to inform BB target pressure
    QString text;
    if (tank == 1){text = "LOX ";}
    else if (tank == 2){text = "ETH ";}
    else if (tank == 6){text = "H2O ";}

    logMessage(globalLogTerminal, text + "BB target set to " + QString::number(pressure) + " bar");
}

// Sends a sequence of integer values (test sequence) and waits for ACK
bool UdpSender::sendsequence(const QList<int> &listvalues) {

    QByteArray data;

    // Header: 4 bytes of 0xAA
    for (int j = 0; j < 4; j++) {
        data.append(static_cast<char>(0xAA));
    }

    // Append each value of the sequence in big-endian order
    for (int value : listvalues) {
        for (int i = 1; i >= 0; i--) {
            data.append(static_cast<char>((value >> (i * 8)) & 0xFF));
        }
    }

    ack = false;
    sendMessage(data);

    // Wait up to 1s for ACK
    QElapsedTimer timer;
    timer.start();
    while (!ack && timer.elapsed() < 1000) {
        QCoreApplication::processEvents();      // Allows Qt to process UDP events
    }

    // If ack is received, return True, else return False
    bool result = ack;
    ack = false;
    return result;
}
