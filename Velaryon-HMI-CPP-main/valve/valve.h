#ifndef VALVE_H
#define VALVE_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

// Valve objects
struct Valve {
    QFrame* frame;
    QLabel* label;
    QLabel* statusLabel;
    QPushButton* openButton;
    QPushButton* closeButton;
    QLabel* valve_type;
    QString name;
    int tableRow;
    int id;
};

extern QMap<QString, Valve> valveMap;
extern QMap<QString, int> valve_Name_ID;

Valve createValve(QWidget* parent, const QString& name, const QPoint& position, const QString& initialStatus, QTableWidget* table, const QString& TabName);


#endif // VALVE_H
