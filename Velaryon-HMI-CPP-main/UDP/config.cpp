#include "UDP/config.h"
#include <QString>


QString Config::computer_ip = "192.168.0.101";      // Test local — remettre 192.168.0.100 pour banc réel
quint16 Config::computer_port = 12345;          // Default port
QString Config::uC_ip = "192.168.0.100";        // Default IP
quint16 Config::uC_port = 5190;                 // Default port - 5190

// Getters for computer IP and port
QString Config::getComputerIpAddress() {
    return computer_ip;
}

quint16 Config::getComputerPort() {
    return computer_port;
}

// Setters for computer IP and port
void Config::setComputerIpAddress(const QString &ip) {
    computer_ip = ip;
}

void Config::setComputerPort(quint16 p) {
    computer_port = p;
}

// Getters for microcontroller IP and port
QString Config::getUC_IpAddress() {
    return uC_ip;
}

quint16 Config::getUC_Port() {
    return uC_port;
}

// Setters for microcontroller IP and port
void Config::setUC_IpAddress(const QString &ip) {
    uC_ip = ip;
}

void Config::setUC_Port(quint16 p) {
    uC_port = p;
}
