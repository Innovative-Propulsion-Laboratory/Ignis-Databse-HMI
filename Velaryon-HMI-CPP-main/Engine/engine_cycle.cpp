/******************************************************************************************
**                                                                                       **
**   EngineTab generates the main content for the "Engine cycle" tab in the user         **
**   interface. This tab includes a graphical representation of the Arrax P&ID,          **
**   dynamic sensor values displayed at their position of the P&ID, a table summarizing  **
**   the valve names and their statuses, and a worker thread that updates sensor values. **                                                                             **
**                                                                                       **
******************************************************************************************/
#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QPixmap>
#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMap>
#include <QFont>
#include <QFrame>
#include <QThread>
#include <QVector>
#include "tools/closeButton.h"
#include "valve/valve.h"
#include "tools/chrono.h"
#include "tools/logo.h"
#include "UDP/udpreceiver.h"
#include "UDP/udpsender.h"
#include "UDP/global_variable.h"
#include "tools/loghelper.h"
#include <QDir>
#include "tools/scaler.h"


QWidget* EngineTab(QWidget* parent,UdpReceiver& receiver) {
    //------------------------------------------------------------------------
    // Create the engine tab widget
    QWidget* engine_tab = new QWidget(parent);

    // Display the Arrax engine image
    QLabel* PID_label = new QLabel(engine_tab);
    QPixmap arrax_engine(":/ressources/pictures/Arrax_engine.png");
    PID_label->setPixmap(arrax_engine);
    PID_label->setScaledContents(true);
    PID_label->setGeometry(scale_x(0), scale_x(0), scale_x(1533), scale_x(767));  // Set position and size

    // Add logo and close button
    logo(engine_tab);
    createCloseButton(engine_tab);

    //------------------------------------------------------------------------
    // Create labels to display dynamic sensor values at specified positions
    QVector<QLabel*> dynamic_sensor_values;
    QVector<QPoint> dynamic_sensor_values_position = {
        {562, 547},
        {1260, 425},
        {562, 151},
        {1260, 279},
        {155, 162},
        {1325, 265},
        {1390, 265},
        {1165, 435},
        {290, 20},
        {1325, 434},
        {1390, 434},
        {807, 417},
        {858,282},
        {290,40}
    };

    // Create 13 labels at the specified positions
    for (int i = 0; i < dynamic_sensor_values_position.size(); ++i) {
        QLabel* label = new QLabel(engine_tab);                     // Create a new label for each sensor value
        label->setGeometry(scale_x(dynamic_sensor_values_position[i].x()), scale_x(dynamic_sensor_values_position[i].y()),
                           scale_x(60), scale_x(21));   // Set position and size
        label->setFont(QFont("Arial", 10, QFont::Bold));            // Set font style
        label->setStyleSheet(R"(background-color: #dcdcdc)");       // Set background color
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);   // Align text to the center
        dynamic_sensor_values.append(label);                        // Add label to the list
    }

    // Connect the label to UdpReceiver for real time display on P&ID
    QObject::connect(&receiver, &UdpReceiver::sensor_value, engine_tab, [dynamic_sensor_values](const QVector<QVariant>& values) {
        int j = 0;

        for (int i = 0; i < values.size() && j < dynamic_sensor_values.size(); ++i) {
            QString text;

            if ( (2 <= i && i <= 5)|| (7 <= i && i <= 9)) {
                text = QString::number(values[i].toDouble(), 'f', 2) + " bar";
            }
            else if (17 <= i && i <= 20) {
                if (i==15){text = "TS12: "+QString::number(values[i].toDouble(), 'f', 2) + " °C";}
                else{text = QString::number(values[i].toDouble(), 'f', 2) + " °C";}
            }
            else if (i == 23 || i == 24) {
                text = QString::number(values[i].toDouble(), 'f', 2) + " L/s";
            }
            else if (i == 27) {
                text = "ADC tension: " + QString::number(values[i].toDouble(), 'f', 2) + "V";
            }
            else {
                continue;
            }
            dynamic_sensor_values[j]->setText(text);
            dynamic_sensor_values[j]->adjustSize();
            j++;
        }
    }, Qt::QueuedConnection);   // Queued to avoid buffer bloating


    //------------------------------------------------------------------------
    // Create the valve status table
    QTableWidget* table_engine = new QTableWidget(0, 2, engine_tab);
    table_engine->setGeometry(scale_x(150), scale_x(400),
                              scale_x(220), scale_x(315));  // Set position and size
    table_engine->setHorizontalHeaderLabels({"Valve Name", "Status"});  // Set header labels

    QFont font = table_engine->horizontalHeader()->font();
    font.setBold(true);  // Make the header font bold
    table_engine->horizontalHeader()->setFont(font);

    table_engine->verticalHeader()->setVisible(false);  // Hide vertical header
    table_engine->setColumnWidth(0, scale_x(120));  // Set column widths
    table_engine->setColumnWidth(1, scale_x(100));
    table_engine->verticalHeader()->setDefaultSectionSize(10);  // Set row height
    table_engine->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Disable vertical scrollbar
    table_engine->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Disable horizontal scrollbar

    table_engine->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table non-editable
    table_engine->setSelectionMode(QAbstractItemView::NoSelection);   // Disable selection
    table_engine->setFocusPolicy(Qt::NoFocus);                        // Remove focus rectangle
    table_engine->setEnabled(false);

    table_engine->setStyleSheet(R"(
            QTableWidget {
                border: 2px solid black;
                background-color: #ededed;
            }
            QHeaderView::section {
                background-color: #dbdbdb;
            }
    )");  // Apply styling to the table

    // Create and position each valve, and register them into the valve status table
    createValve(engine_tab, "SV11", QPoint(scale_x(715), scale_x(523)), "Open", table_engine, "T1");
    createValve(engine_tab, "SV12", QPoint(scale_x(1010), scale_x(435)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV13", QPoint(scale_x(1012), scale_x(578)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV21", QPoint(scale_x(712), scale_x(117)), "Open", table_engine, "T1");
    createValve(engine_tab, "SV22", QPoint(scale_x(975), scale_x(205)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV24", QPoint(scale_x(1012), scale_x(60)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV31", QPoint(scale_x(110), scale_x(237)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV32", QPoint(scale_x(322), scale_x(210)), "Open", table_engine, "T1");
    createValve(engine_tab, "SV33", QPoint(scale_x(519), scale_x(435)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV34", QPoint(scale_x(519), scale_x(200)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV35", QPoint(scale_x(1260), scale_x(159)), "Close", table_engine, "T1");
    createValve(engine_tab, "SV36", QPoint(scale_x(1260), scale_x(480)), "Close", table_engine, "T1");

    // Add chrono (timer) to the engine tab
    chrono(engine_tab);
    
    // Button to reconnect the uC and control valves
    QPushButton* reconnect_uC = new QPushButton("Initialise valves", engine_tab);
    reconnect_uC->setGeometry(scale_x(50), scale_x(100),
                              scale_x(100), scale_x(30));
    reconnect_uC->setFont(QFont("Arial", 8, QFont::Bold));
    reconnect_uC->setStyleSheet(R"(
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

    // Isolate the GN2 and air tanks to avoid loss
    QObject::connect(reconnect_uC, &QPushButton::clicked, engine_tab, []() {
        UdpSender* sender = new UdpSender();

        QByteArray data;
        for (int j = 0; j < 4; j++) {
            data.append(static_cast<char>(0xFF));   // Append 0xFF four times
        }

        // LOX line
        data.append(static_cast<char>(0 & 0xFF));   // Append valve ID
        data.append(static_cast<char>(0));          // Append 1 if open, 0 if closed
        sender->sendMessage(data);
        QThread::msleep(10);
        logMessage(globalLogTerminal, "SV11 closed");

        // ETH line
        data[4] = 5;
        sender->sendMessage(data);
        QThread::msleep(10);
        logMessage(globalLogTerminal, "SV21 closed");

        // GN2 line
        data[4] = 8;
        data[5] = 1;
        sender->sendMessage(data);
        QThread::msleep(10);
        logMessage(globalLogTerminal, "SV31 opened");

        data[4] = 9;
        data[5] = 0;
        sender->sendMessage(data);
        QThread::msleep(10);
        logMessage(globalLogTerminal, "SV32 closed");

        // Air line
        data[4] = 14;
        sender->sendMessage(data);
        QThread::msleep(10);
        logMessage(globalLogTerminal, "SV51 closed");

        // H2O line
        data[4] = 17;
        sender->sendMessage(data);
        QThread::msleep(10);
        logMessage(globalLogTerminal, "SV61 closed");

        data[4] = 18;
        sender->sendMessage(data);
        logMessage(globalLogTerminal, "SV62 closed");
    });

    // Return the populated engine tab widget
    return engine_tab;
}
