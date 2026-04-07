#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMap>
#include <QFont>
#include <QFrame>
#include <QDebug>
#include "glowplug.h"
#include "UDP/udpsender.h"
#include <QTimer>
#include "tools/global_variable.h"
#include "tools/loghelper.h"

// Map to store Glowplugs with their names as keys
QMap<QString, Glowplug> glowplugMap;

// Create, place gowplug object & connect it to sendCommand
Glowplug createGlowplug(QWidget* parent, const QString& name, const QPoint& position, const QString& initialStatus, QTableWidget* table) {
    Glowplug glowplug;

    //-------------------------------------------------------------------
    // Control frame
    glowplug.frame = new QFrame(parent);
    glowplug.frame->setGeometry(position.x(), position.y(), 128, 85);
    glowplug.frame->setObjectName("glowplugFrame");
    glowplug.frame->setStyleSheet(R"(
        QFrame#glowplugFrame {
            background-color: rgb(230, 230, 230);
            border: 3px solid grey;
            border-radius: 10px;
            padding: 2px;
        }
    )");
    //-------------------------------------------------------------------
    // On and off buttons
    QFont font = QFont("Arial", 8, QFont::Bold);
    glowplug.onButton = new QPushButton("On", glowplug.frame);
    glowplug.onButton->setGeometry(17, 50, 40, 25);
    glowplug.onButton->setFont(font);
    glowplug.onButton->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 3px;
            height: 30px;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px rgb(255,200,200);
        }
    )");

    glowplug.offButton = new QPushButton("Off", glowplug.frame);
    glowplug.offButton->setGeometry(69, 50, 40, 25);
    glowplug.offButton->setFont(font);
    glowplug.offButton->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 3px;
            height: 30px;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px rgb(255,200,200);
        }
    )");
    //-------------------------------------------------------------------
    // Label for glowplug name
    glowplug.label = new QLabel(name + " status:", glowplug.frame);
    glowplug.label->setGeometry(20, 10, 100, 16);
    font.setUnderline(true);
    glowplug.label->setFont(font);
    glowplug.label->setStyleSheet("QLabel { color: black; }");

    //-------------------------------------------------------------------
    // Label for glowplug status
    glowplug.statusLabel = new QLabel(initialStatus, glowplug.frame);
    glowplug.statusLabel->setGeometry(55, 26, 70, 16);
    font.setUnderline(false);
    glowplug.statusLabel->setFont(font);
    glowplug.statusLabel->setStyleSheet(initialStatus == "On" ? "QLabel { color: green; }" : "QLabel { color: red; }");

    //-------------------------------------------------------------------
    // Add glowplug to the overview table and save the row index
    int row = table->rowCount();
    table->insertRow(row);

    // Add name of the glowplug in first column
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);

    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFont(font);
    nameItem->setForeground(QColor("black"));
    table->setItem(row, 0, nameItem);

    // Add state of the glowplug in second column
    QTableWidgetItem* stateItem = new QTableWidgetItem(initialStatus);
    stateItem->setTextAlignment(Qt::AlignCenter);
    stateItem->setFont(font);
    stateItem->setForeground(initialStatus == "On" ? QColor("green") : QColor("red"));
    table->setItem(row, 1, stateItem);

    // Add glowplug in the map
    glowplug.tableRow = row;
    glowplugMap[name] = glowplug;

    // ID for easier management
    static int id = 0;
    glowplug.id = id;
    id++;

    // Connect glowplug on/off buttons to send glowplug command over UDP
    UdpSender* sender = new UdpSender();
    QObject::connect(glowplug.onButton, &QPushButton::clicked, [=]() { sender->sendGlowplug(glowplug, true); });
    QObject::connect(glowplug.offButton, &QPushButton::clicked, [=]() { sender->sendGlowplug(glowplug, false); });
    return glowplug;
}
