#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "UDP/udpreceiver.h"
#include "Database/database_manager.h"

QMainWindow* createMainWindow(UdpReceiver& receiver, DatabaseManager* dbManager = nullptr);

#endif // MAINWINDOW_H
