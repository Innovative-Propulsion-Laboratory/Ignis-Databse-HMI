/******************************************************************************************
**                                                                                       **
**   CoolingTab generates the main content for the "Cooling cycle" tab in the user       **
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
#include "UDP/global_variable.h"
#include "tools/loghelper.h"
#include "tools/scaler.h"

QWidget* CoolingTab(QWidget* parent,UdpReceiver& receiver) {
    //------------------------------------------------------------------------
    // Create the cooling tab widget
    QWidget* cooling_tab = new QWidget(parent);

    // Display the Arrax cooling image
    QLabel* PID_label = new QLabel(cooling_tab);
    QPixmap arrax_cooling(":/ressources/pictures/Arrax_cooling.png");
    PID_label->setPixmap(arrax_cooling);
    PID_label->setScaledContents(true);
    PID_label->setGeometry(scale_x(0), scale_x(0),
                           scale_x(1533), scale_x(767));  // Set position and size

    // Add logo and close button
    logo(cooling_tab);
    createCloseButton(cooling_tab);


    //------------------------------------------------------------------------
    // Create labels to display dynamic sensor values at specified positions
    QVector<QLabel*> dynamic_sensor_values;
    QVector<QPoint> dynamic_sensor_values_position = {
        {176, 157},
        {577, 550},
        {577, 157},
        {1106, 295},
        {1239, 295},
        {1106, 410},
        {1239, 410},
        {1023, 310},
        {290,20}
    };

    // Create 8 labels at the specified positions
    for (int i = 0; i < dynamic_sensor_values_position.size(); ++i) {
        QLabel* label = new QLabel(cooling_tab);  // Create a new label for each sensor value
        label->setGeometry(scale_x(dynamic_sensor_values_position[i].x()), scale_x(dynamic_sensor_values_position[i].y()),
                           scale_x(60), scale_x(21));  // Set position and size
        label->setFont(QFont("Arial",10,QFont::Bold));  // Set font style
        label->setStyleSheet(R"(background-color: #dcdcdc)");  // Set background color
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  // Align text to the center
        dynamic_sensor_values.append(label);  // Add label to the list
    }

    // Update labels when new values are obtained by the worker
    QObject::connect(&receiver, &UdpReceiver::sensor_value, cooling_tab, [dynamic_sensor_values](const QVector<QVariant>& values) {
        int j = 0;

        for (int i = 0; i < values.size() && j < dynamic_sensor_values.size(); ++i) {
            QString text;

            if (10 <= i && i <= 14) {
                text = QString::number(values[i].toDouble(), 'f', 2) + " bar";
            }
            else if (i == 21 || i == 22) {
                text = QString::number(values[i].toDouble(), 'f', 2) + " °C";
            }
            else if (i == 25) {
                text = QString::number(values[i].toDouble(), 'f', 2) + " L/s";
            }
            else if (i == 27) {
                text = "ADC tension: " + QString::number(values[i].toDouble(), 'f', 2) + "V";
            }
            else {
                continue;  // Skip irrelevant indices
            }


            //qDebug()<<"cooling"<<j<<dynamic_sensor_values.size();
            dynamic_sensor_values[j]->setText(text);
            dynamic_sensor_values[j]->adjustSize();
            j++;
        }
    },
                     Qt::QueuedConnection);



    //------------------------------------------------------------------------
    // Create the valve status table
    QTableWidget* table_cooling = new QTableWidget(0, 2, cooling_tab);
    table_cooling->setGeometry(scale_x(200),scale_x(420),
                               scale_x(220), scale_x(170));  // Set position and size
    table_cooling->setHorizontalHeaderLabels({"Valve Name", "Status"});  // Set header labels

    QFont font = table_cooling->horizontalHeader()->font();
    font.setBold(true);  // Make the header font bold
    table_cooling->horizontalHeader()->setFont(font);

    table_cooling->verticalHeader()->setVisible(false);  // Hide vertical header
    table_cooling->setColumnWidth(0, scale_x(120));  // Set column widths
    table_cooling->setColumnWidth(1, scale_x(100));
    table_cooling->verticalHeader()->setDefaultSectionSize(10);  // Set row height
    table_cooling->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Disable vertical scrollbar
    table_cooling->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Disable horizontal scrollbar
    table_cooling->setEnabled(false);

    table_cooling->setStyleSheet(R"(
            QTableWidget {
                border: 2px solid black;
                background-color: #ededed;
            }
            QHeaderView::section{
                background-color: #dbdbdb;
            }
    )");  // Apply styling to the table

    // Create and position each valve, and register them into the valve status table
    createValve(cooling_tab, "SV51", QPoint(scale_x(310), scale_x(195)), "Open", table_cooling, "T2");
    createValve(cooling_tab, "SV52", QPoint(scale_x(465),scale_x(450)), "Close", table_cooling, "T2");
    createValve(cooling_tab, "SV53", QPoint(scale_x(465), scale_x(190)), "Close", table_cooling, "T2");
    createValve(cooling_tab, "SV61", QPoint(scale_x(752), scale_x(518)), "Open", table_cooling, "T2");
    createValve(cooling_tab, "SV62", QPoint(scale_x(752), scale_x(120)), "Open", table_cooling, "T2");
    createValve(cooling_tab, "SV63", QPoint(scale_x(904), scale_x(227)), "Close", table_cooling, "T2");

    // Add chrono (timer) to the engine tab
    chrono(cooling_tab);

    // Button to reconnect the uC and control valves
    QPushButton* reconnect_uC = new QPushButton("Initialise valves", cooling_tab);
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
    QObject::connect(reconnect_uC, &QPushButton::clicked, cooling_tab, []() {
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


    // Return the populated cooling tab widget
    return cooling_tab;
}

