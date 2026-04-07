#ifndef TEST_H
#define TEST_H

#include <QWidget>
#include <QTabWidget>
#include <QComboBox>
#include "UDP/udpreceiver.h"
#include "Database/database_manager.h"
#include <QPlainTextEdit>

void update_combobox(QComboBox* comboBox);

QWidget* TestTab(QWidget* parent, QTabWidget* tabwidget, UdpReceiver& receiver, DatabaseManager* dbManager = nullptr);


#endif // TEST_H
