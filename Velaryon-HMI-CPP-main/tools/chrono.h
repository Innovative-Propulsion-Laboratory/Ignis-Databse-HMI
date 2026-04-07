#ifndef CHRONO_H
#define CHRONO_H

#include <QLabel>
#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include "tools/scaler.h"

inline void chrono(QWidget* parent) {
    // Create the date and time label
    QLabel* dateTimeLabel = new QLabel(parent);
    dateTimeLabel->setGeometry(scale_x(20), scale_x(10), scale_x(250), scale_x(30));
    dateTimeLabel->setFont(QFont("Arial", 10, QFont::Bold));
    dateTimeLabel->setStyleSheet("QLabel { color: black; }");

    // Create the chrono label
    QLabel* chronoLabel = new QLabel(parent);
    chronoLabel->setGeometry(scale_x(50), scale_x(50),
                             scale_x(250), scale_x(30));
    chronoLabel->setFont(QFont("Arial", 10, QFont::Bold));
    chronoLabel->setStyleSheet("QLabel { color: black; }");

    // Store the launch time
    QDateTime launchTime = QDateTime::currentDateTime();

    // Create the timer with the parent to ensure proper ownership
    QTimer* timer = new QTimer(parent);

    QObject::connect(timer, &QTimer::timeout, [=]() {
        // Update current date and time
        QDateTime currentDateTime = QDateTime::currentDateTime();
        dateTimeLabel->setText("Date: " + currentDateTime.toString("yyyy/MM/dd") +
                               " Time: " + currentDateTime.toString("hh:mm:ss"));

        // Calculate elapsed time since launch
        qint64 elapsedSecs = launchTime.secsTo(currentDateTime);
        QTime elapsedTime(0, 0); // Start from 00:00:00
        elapsedTime = elapsedTime.addSecs(elapsedSecs);
        QString chronoString = "IHM uptime: " + elapsedTime.toString("hh:mm:ss");
        chronoLabel->setText(chronoString);
    });

    // Start the timer
    timer->start(100);  // Update every 100ms
}

#endif // CHRONO_H
