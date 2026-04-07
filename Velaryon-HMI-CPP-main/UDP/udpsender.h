#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include "Glowplug/glowplug.h"
#include "valve/valve.h"

// UdpSender handles sending UDP messages for valves, TVC, and other controls
class UdpSender : public QObject {
    Q_OBJECT

public:
    explicit UdpSender(QObject *parent = nullptr);
    void sendMessage(const QByteArray &data);
    void sendValve(Valve valve, bool open);
    void sendGlowplug(Glowplug glowplug, bool on);
    void sendTVCtest(int shape);
    bool sendBBcontrol(int tank, int activate);
    void sendsetpressure(int tank, float pressure);
    bool sendsequence(const QList<int> &listvalues);

private:
    QUdpSocket udpSocket;       // UDP socket used to send datagrams
    QHostAddress destinationIp; // IP address of the target device
    quint16 destinationPort;    // Destination UDP port number
};

#endif // UDPSENDER_H
