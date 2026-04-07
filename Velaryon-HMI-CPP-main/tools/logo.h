#ifndef LOGO_H
#define LOGO_H

#include <QLabel>
#include <QWidget>
#include <QPixmap>
#include "tools/scaler.h"

// Get logo and set dimensions
inline QLabel* logo(QWidget* tab){
    QLabel* logo = new QLabel(tab);
    QPixmap pixmap(":/ressources/pictures/Logo_IPL.png");
    logo->setPixmap(pixmap);
    logo->setScaledContents(true);
    logo->setGeometry(scale_x(1280), scale_x(50),
                      scale_x(200), scale_x(80));
    return logo;
}

#endif // LOGO_H
