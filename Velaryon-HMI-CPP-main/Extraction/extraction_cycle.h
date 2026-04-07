#ifndef EXTRACTION_CYCLE_H
#define EXTRACTION_CYCLE_H

#include <QMainWindow>
#include "UDP/udpreceiver.h"
#include "Database/database_manager.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QRadioButton>
#include <QStackedWidget>
#include <QDateTimeEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QCheckBox>
#include <QSet>
#include <QColor>

QWidget* ExtractionTab(QWidget* parent, UdpReceiver& receiver, DatabaseManager* dbManager = nullptr);
void setAllSensorsState(QTreeWidget* tree, Qt::CheckState state);
#endif // EXTRACTION_CYCLE_H
