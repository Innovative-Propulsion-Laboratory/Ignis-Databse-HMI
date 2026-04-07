#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config {
public:
    static QString getIpAddress();
    static quint16 getPort();

    // Getters for computer IP and port
    static QString getComputerIpAddress();
    static quint16 getComputerPort();

    // Setters for computer IP and port
    static void setComputerIpAddress(const QString &ip);

    static void setComputerPort(quint16 p);

    // Getters for microcontroller IP and port
    static QString getUC_IpAddress();

    static quint16 getUC_Port();

    // Setters for microcontroller IP and port
    static void setUC_IpAddress(const QString &ip);

    static void setUC_Port(quint16 p);

private:
    static QString computer_ip;
    static quint16 computer_port;
    static QString uC_ip;
    static quint16 uC_port;
};

#endif // CONFIG_H
