/*******************************************************************************
**                                                                            **
**   Velaryon is a modern and intuitive Human-Machine Interface (HMI)         **
**   designed to facilitate control and monitoring for the Arrax student      **
**   rocket engine project.                                                   **
**                                                                            **
**   This software provides a user-friendly interface for engineers and       **
**   students working on the development and testing of the Arrax rocket      **
**   engine at the Innovative Propulsion Laboratory (IPL).                    **
**                                                                            **
**   This software is the intellectual property of Innovative Propulsion      **
**   Laboratory and is intended for educational and research purposes only.   **
**                                                                            **
********************************************************************************
**           Author: Mehdi Delouane                                           **
**           Contact: mehdi.delouane@ipsa.fr                                  **
**             Date: 19.02.2025                                               **
**             Version: 2.2.0                                                 **
********************************************************************************
**  This version was designed specifically for the Arrax project.             **
**  You may use this version as a reference but do not modify it directly.    **
*******************************************************************************/

#include <QApplication>
#include <QMainWindow>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <tools/loading_screen.h>
#include <mainwindow.h>
#include <UDP/udpreceiver.h>
#include <UDP/udpsender.h>
#include <Database/database_manager.h>
// #include <Database/test_data_generator.h> // ABANDONNÉ & Remplacé par le simulateur Python sender_test
#include <QDebug>

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);

    // ---- Initialisation de la base de données ----
    DatabaseManager dbManager;

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                     + "/Velaryon-HMI/velaryon_data.db";

    // Crée le dossier si nécessaire
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    if (!dbManager.openDatabase(dbPath))
        qWarning() << "[main] Impossible d'ouvrir la base de données :" << dbPath;
    else
        qDebug() << "[main] Base de données ouverte :" << dbPath;

    dbManager.closeOrphanTests();
    // ---------------------------------------------------------------------------------
    // ---- TEST UNIQUEMENT (ABANDONNÉ;) : remplacé par le simulateur Python sender_test ----
    // TestDataGenerator::generateFakeSession(&dbManager, 2000, true);
    // ---------------------------------------------------------------------------------

    UdpReceiver receiver;

    // Branche le DatabaseManager sur le receiver UDP
    receiver.setDatabaseManager(&dbManager);

    // Init Teensy with computer IP
    UdpSender sender;       //à changer
    QByteArray data;
    data.append(static_cast<char>(0x00));
    sender.sendMessage(data);    //à changer

    // Create the main window with tabs
    QMainWindow* mainWindow = createMainWindow(receiver, &dbManager);
    mainWindow->hide();  // Initially hide the main window

    // Create and show the loading screen
    QWidget* loadingScreen = createLoadingScreen(mainWindow);
    loadingScreen->show();
    return app.exec();
}
