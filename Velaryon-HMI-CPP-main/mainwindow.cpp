/*****************************************************************************************
**                                                                                      **
**   createMainWindow generates a main window with a QTabWidget containing four tabs:   **
**   Engine cycle, Cooling cycle, Test, and Graphic.                                    **
**                                                                                      **
**   Each tab focuses on a specific aspect of the project, providing a clear and        **
**   organized way to divide the content.                                               **
**   This approach helps users concentrate on one area at a time.                       **
**                                                                                      **
*****************************************************************************************/
#include <QMainWindow>
#include "Engine/engine_cycle.h"
#include "Cooling/cooling_cycle.h"
#include "Test/test.h"
#include "Graphic/graphic.h"
#include "UDP/udpreceiver.h"
#include "tools/scaler.h"
#include "Igniter/igniter_cycle.h"
#include "Extraction/extraction_cycle.h"


QMainWindow* createMainWindow(UdpReceiver& receiver, DatabaseManager* dbManager){
    // Create a new QMainWindow instance
    QMainWindow* mainWindow = new QMainWindow();

    // Set the window to be borderless and maximized (pseudo-fullscreen)
    mainWindow->setWindowFlags(Qt::FramelessWindowHint);
    mainWindow->showMaximized();  // Maximize the window on startup

    // Create a QTabWidget to hold tabs for different sections
    QTabWidget* tabWidget = new QTabWidget(mainWindow);

    // Customize the tab widget's appearance with a stylesheet
    tabWidget->setStyleSheet(R"(
    QTabWidget::pane { border: 1px solid #000000; }
    QTabBar::tab {
        background-color: #bcbaba; color: black; height: 40px; width: 150px; font-weight: bold;
        border: 1px solid #000000;
    }
    QTabBar::tab:hover { background-color: #a8a8a8; }
    QTabBar::tab:selected { background-color: #d4d4d4; }
)");
    setScreenSize(mainWindow);
    // Add tabs to the tab widget with associated content
    tabWidget->addTab(EngineTab(mainWindow,receiver), "Engine cycle");      // Engine cycle tab
    tabWidget->addTab(CoolingTab(mainWindow, receiver), "Cooling cycle");   // Cooling cycle tab
    tabWidget->addTab(IgniterTab(mainWindow, receiver), "Igniter cycle");    // Igniter cycle tab
    tabWidget->addTab(TestTab(mainWindow, tabWidget, receiver, dbManager), "Test");     // Test tab
    tabWidget->addTab(Graphic(mainWindow,receiver), "Graphic");             // Graphic tab
    tabWidget->addTab(ExtractionTab(mainWindow, receiver, dbManager), "Extraction");   // Extraction cycle tab


    // Set the QTabWidget as the central widget of the main window
    mainWindow->setCentralWidget(tabWidget);

    // Return the main window with all components set up
    return mainWindow;
}
