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
#include "glowplug/glowplug.h"
#include "tools/chrono.h"
#include "tools/logo.h"
#include "UDP/udpreceiver.h"
#include "UDP/udpsender.h"
#include "UDP/global_variable.h"
#include "tools/loghelper.h"
#include <QDir>


QWidget* IgniterTab(QWidget* parent,UdpReceiver& receiver) {
    //------------------------------------------------------------------------
    // Create the igniter tab widget
    QWidget* igniter_tab = new QWidget(parent);

    // Display the Arrax igniter image
    QLabel* PID_label = new QLabel(igniter_tab);
    QPixmap arrax_igniter(":/ressources/pictures/Arrax_igniter.png");
    PID_label->setPixmap(arrax_igniter);
    PID_label->setScaledContents(true);
    PID_label->setGeometry(0, 0, 1498, 807);  // Set position and size

    // Add logo and close button
    logo(igniter_tab);
    createCloseButton(igniter_tab);

    //------------------------------------------------------------------------
    // Create labels to display dynamic sensor values at specified positions
    QVector<QLabel*> dynamic_sensor_values;
    QVector<QPoint> dynamic_sensor_values_position = {
        {650, 130},     //PS21
        {1205, 427},    //PS23
        {172, 197},     //PS31
        {1205, 703},    //PS71
        {1260, 460},    //PS81
        {1105, 520}     //Glowplug
    };

    // Create 6 labels at the specified positions
    for (int i = 0; i < dynamic_sensor_values_position.size(); ++i) {
        QLabel* label = new QLabel(igniter_tab);                     // Create a new label for each sensor value
        label->setGeometry(dynamic_sensor_values_position[i].x(), dynamic_sensor_values_position[i].y(),
                           60, 21);   // Set position and size
        label->setFont(QFont("Arial", 10, QFont::Bold));            // Set font style
        label->setStyleSheet(R"(background-color: #dcdcdc)");       // Set background color
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);   // Align text to the center
        dynamic_sensor_values.append(label);                        // Add label to the list
    }


    // Connect the label to UdpReceiver for real time display on P&ID
    QObject::connect(&receiver, &UdpReceiver::sensor_value, igniter_tab, [dynamic_sensor_values](const QVector<QVariant>& values) {
        int j = 0;

        for (int i = 0; i < values.size() && j < dynamic_sensor_values.size(); ++i) {
            QString text;

            if ( (4 == i) || (6 <= i && i <= 7) || (15 <= i && i <= 16) ) {
                text = QString::number(values[i].toDouble(), 'f', 2) + " bar";
            }
            else if (i == 28) {
                text = "GP " + QString::number(values[i].toDouble(), 'f', 2) + "A";
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
    QTableWidget* table_igniter = new QTableWidget(0, 2, igniter_tab);
    table_igniter->setGeometry(scale_x(200), scale_x(490),
                               scale_x(220), scale_x(315));  // Set position and size
    table_igniter->setHorizontalHeaderLabels({"Valve Name", "Status"});  // Set header labels

    QFont font = table_igniter->horizontalHeader()->font();
    font.setBold(true);  // Make the header font bold
    table_igniter->horizontalHeader()->setFont(font);

    table_igniter->verticalHeader()->setVisible(false);  // Hide vertical header
    table_igniter->setColumnWidth(0, scale_x(120));  // Set column widths
    table_igniter->setColumnWidth(1, scale_x(100));
    table_igniter->verticalHeader()->setDefaultSectionSize(10);  // Set row height
    table_igniter->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Disable vertical scrollbar
    table_igniter->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Disable horizontal scrollbar

    table_igniter->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table non-editable
    table_igniter->setSelectionMode(QAbstractItemView::NoSelection);   // Disable selection
    table_igniter->setFocusPolicy(Qt::NoFocus);                        // Remove focus rectangle
    table_igniter->setEnabled(false);

    table_igniter->setStyleSheet(R"(
            QTableWidget {
                border: 2px solid black;
                background-color: #ededed;
            }
            QHeaderView::section {
                background-color: #dbdbdb;
            }
    )");  // Apply styling to the table

    // Create and position each valve, and register them into the valve status table
    createValve(igniter_tab, "SV21", QPoint(710,225), "Open", table_igniter, "T3");
    createValve(igniter_tab, "SV22", QPoint(880,340), "Close", table_igniter, "T3");
    createValve(igniter_tab, "SV24", QPoint(905,130), "Close", table_igniter, "T3");
    createValve(igniter_tab, "SV25", QPoint(1030,360), "Close", table_igniter, "T3");
    createValve(igniter_tab, "SV31", QPoint(15,240), "Close", table_igniter, "T3");
    createValve(igniter_tab, "SV32", QPoint(260,375), "Open", table_igniter, "T3");
    createValve(igniter_tab, "SV33", QPoint(432,445), "Close", table_igniter, "T3");
    createValve(igniter_tab, "SV34", QPoint(432,240), "Close", table_igniter, "T3");
    createValve(igniter_tab, "SV35", QPoint(1280,160), "Close", table_igniter, "T3");
    createValve(igniter_tab, "SV71", QPoint(1030,670), "Close", table_igniter, "T3");
    createGlowplug(igniter_tab, "Glowplug", QPoint(960,518), "Off", table_igniter);

    // Add chrono (timer) to the igniter tab
    chrono(igniter_tab);

    // Button to reconnect the uC and control valves
    QPushButton* reconnect_uC = new QPushButton("Initialise valves", igniter_tab);
    reconnect_uC->setGeometry(scale_x(50), scale_x(100),
                              scale_x(100),scale_x(30));
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
    QObject::connect(reconnect_uC, &QPushButton::clicked, igniter_tab, []() {
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


    // Return the populated igniter tab widget
    return igniter_tab;
}
