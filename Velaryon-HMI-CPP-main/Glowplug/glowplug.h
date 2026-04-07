#ifndef GLOWPLUG_H
#define GLOWPLUG_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

// Glowplug objects
struct Glowplug {
    QFrame* frame;
    QLabel* label;
    QLabel* statusLabel;
    QPushButton* onButton;
    QPushButton* offButton;
    QLabel* type;
    int tableRow;
    int id;
};

extern QMap<QString, Glowplug> glowplugMap;

Glowplug createGlowplug(QWidget* parent, const QString& name, const QPoint& position, const QString& initialStatus, QTableWidget* table);

#endif // GLOWPLUG_H
