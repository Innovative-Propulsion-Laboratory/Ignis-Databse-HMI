/*****************************************************************************************
**                                                                                      **
**   createValve builds the valve control frame with open and close buttons, adds       **
**   labels for the valve name, type, and current status, inserts the valve entry       **
**   into the overview table, registers the valve in the global valve map for           **
**   tracking, connects the button actions to UDP commands using UdpSender, and         **
**   starts a periodic timer to keep the table synchronized with the valve state.       **
**                                                                                      **
*****************************************************************************************/
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
#include "valve/valve.h"
#include "UDP/udpsender.h"
#include <QTimer>
#include "tools/global_variable.h"
#include "tools/loghelper.h"


// Map to store valves with their names as keys
// QMap<QString, QPair<Valve, int>> valveMap;
QMap<QString, Valve> valveMap;
QMap<QString, int> valve_Name_ID = {{"SV11",0},{"SV12",1},{"SV13",2},{"SV21",5},
                                    {"SV22",6},{"SV24",3},{"SV25",7},{"SV31",8},{"SV32",9},
                                    {"SV33",10},{"SV34",11},{"SV35",12},{"SV36",13},
                                    {"SV51",14},{"SV52",15},{"SV53",16},{"SV61",17},
                                    {"SV62",18},{"SV63",19},{"SV71",4}};
QMap<QTableWidget*, QTimer*> tableUpdateTimers;

// Create, place valve object & connect it to sendCommand
Valve createValve(QWidget* parent, const QString& name, const QPoint& position, const QString& initialStatus, QTableWidget* table, const QString& TabName) {
    Valve valve;

    //-------------------------------------------------------------------
    // Control frame
    valve.frame = new QFrame(parent);
    valve.frame->setGeometry(position.x(), position.y(), 105, 85);
    valve.frame->setObjectName("valveFrame");
    valve.frame->setStyleSheet(R"(
        QFrame#valveFrame {
            background-color: rgb(230, 230, 230);
            border: 3px solid grey;
            border-radius: 10px;
            padding: 2px;
        }
    )");
    //-------------------------------------------------------------------
    // Open and close buttons
    QFont font = QFont("Arial", 8, QFont::Bold);
    valve.openButton = new QPushButton("Open", valve.frame);
    valve.openButton->setGeometry(10, 50, 40, 25);
    valve.openButton->setFont(font);
    valve.openButton->setStyleSheet(R"(
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

    valve.closeButton = new QPushButton("Close", valve.frame);
    valve.closeButton->setGeometry(55, 50, 40, 25);
    valve.closeButton->setFont(font);
    valve.closeButton->setStyleSheet(R"(
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
    // Label for valve name
    valve.label = new QLabel(name + " status:", valve.frame);
    valve.label->setGeometry(15, 10, 70, 16);
    font.setUnderline(true);
    valve.label->setFont(font);
    valve.label->setStyleSheet("QLabel { color: black; }");

    //-------------------------------------------------------------------
    // Label for valve status
    valve.statusLabel = new QLabel(initialStatus, valve.frame);
    valve.statusLabel->setGeometry(35, 26, 70, 16);
    font.setUnderline(false);
    valve.statusLabel->setFont(font);
    valve.statusLabel->setStyleSheet(initialStatus == "Open" ? "QLabel { color: green; }" : "QLabel { color: red; }");

    //-------------------------------------------------------------------
    // Label for valve type
    QFont font2 = QFont("Arial", 7, QFont::Bold);
    valve.valve_type = new QLabel(initialStatus == "Open" ? "NO" : "NC", valve.frame);
    valve.valve_type->setGeometry(85, 0, 70, 16);
    font.setUnderline(false);
    valve.valve_type->setFont(font2);
    valve.valve_type->setStyleSheet("QLabel { color: #A8A8A8; }");

    //-------------------------------------------------------------------
    // Add valve to the overview table and save the row index
    int row = table->rowCount();
    table->insertRow(row);

    // Add name of the valve in first column
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);

    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFont(font);
    nameItem->setForeground(QColor("black"));
    table->setItem(row, 0, nameItem);

    // Add state of the valve in second column
    QTableWidgetItem* stateItem = new QTableWidgetItem(initialStatus);
    stateItem->setTextAlignment(Qt::AlignCenter);
    stateItem->setFont(font);
    stateItem->setForeground(initialStatus == "Open" ? QColor("green") : QColor("red"));
    table->setItem(row, 1, stateItem);

    // Add Name for log

    valve.name = name;

    // Add valve in the map
    valve.tableRow = row;

    QString key = TabName + "::" + name;
    valveMap[key] = valve;


    //ID for easier management
    valve.id = valve_Name_ID[name];
    // qDebug()<<valve.id;



    // Connect valve open/close buttons to send valve command over UDP
    UdpSender* sender = new UdpSender();
    QObject::connect(valve.openButton, &QPushButton::clicked, [=]() { sender->sendValve(valve, true); });
    QObject::connect(valve.closeButton, &QPushButton::clicked, [=]() { sender->sendValve(valve, false); });


    // Create an update timer linked to table
    if (!tableUpdateTimers.contains(table)) {
        QTimer* updateTimer = new QTimer(parent);
        tableUpdateTimers[table] = updateTimer;
        // Periodically update the table (only) with the latest valve states
        QObject::connect(updateTimer, &QTimer::timeout, [=]() {
            // qDebug()<<"----------------------------";


            for (const QString& key : valveMap.keys()) {
                const Valve &valve = valveMap[key];

                // qDebug()<<key;

                // Skip valves not linked to this table/tab
                if (valve.tableRow < 0 || !valve.statusLabel || valve.frame->parentWidget() != table->parentWidget())
                    continue;

                QTableWidgetItem* item = table->item(valve.tableRow, 1);
                // qDebug()<<key<<valve.tableRow;
                QString labelStatus = valve.statusLabel->text();


                if (item && item->text() != labelStatus) {
                    // qDebug()<<item;
                    item->setText(labelStatus);
                    item->setForeground(labelStatus == "Open" ? QColor("green") : QColor("red"));
                }
            }
        });

        // Start periodic update every 100 ms
        updateTimer->start(100);
    }

    return valve;
}

