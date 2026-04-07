#ifndef CLOSEBUTTON_H
#define CLOSEBUTTON_H

#include <QPushButton>
#include <QCoreApplication>
#include <QMessageBox>
#include "tools/scaler.h"

// Close button to remove window control buttons
inline QPushButton* createCloseButton(QWidget* parent) {
    QPushButton* button = new QPushButton("End program", parent);
    button->setGeometry(scale_x(1350), scale_x(690), scale_x(120), scale_x(40));
    button->setStyleSheet(R"(
        QPushButton {
            border: 2px solid #B20000;
            background-color: #FF1400;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: rgb(255,100,100);
            border: 2px solid rgb(255,200,200);
        }
    )");
    button->setFont(QFont("Arial", 10, QFont::Bold));

    QObject::connect(button, &QPushButton::clicked, [=]() {
        // Create a warning message box
        QMessageBox::StandardButton reply = QMessageBox::warning(
            parent, "Confirm Exit", "Are you sure you want to exit?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No
            );

        // If Yes is clicked, exit the application
        if (reply == QMessageBox::Yes) {
            QCoreApplication::quit();
        }
    });

    return button;
}

#endif // CLOSEBUTTON_H
