/*************************************************************************************
**                                                                                  **
**   createLoadingScreen generates a loading screen with a logo and progress bar.   **
**   It simulates a loading process by updating the progress bar value and          **
**   displays the main window once the loading is complete.                         **
**                                                                                  **
*************************************************************************************/
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QMainWindow>
#include <QDir>
#include <tools/closeButton.h>
#include "tools/scaler.h"

QWidget* createLoadingScreen(QMainWindow* mainWindow) {

    // Create the loading screen widget
    QWidget* loadingScreen = new QWidget();
    loadingScreen->setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);  // No window frame and splash screen style
    loadingScreen->resize(320, 320);  // Set size of the loading screen

    // Create a vertical layout for the loading screen
    QVBoxLayout* layout = new QVBoxLayout(loadingScreen);

    // Image label to display logo
    QLabel* imageLabel = new QLabel();

    // Load and set logo image from the directory path
    QPixmap pixmap(":/ressources/pictures/Logo_Arrax.png");        // Update with your image path
    imageLabel->setPixmap(pixmap.scaled(scale_x(320), scale_x(320), Qt::KeepAspectRatio, Qt::SmoothTransformation));  // Scale image
    layout->addWidget(imageLabel, 0, Qt::AlignCenter);  // Add image to the layout and center it

    // Create a progress bar to show loading progress
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);      // Set range of progress bar
    progressBar->setValue(0);           // Initialize progress bar to 0
    progressBar->setStyleSheet(R"(
            QProgressBar {
                border: 2px solid #555;
                border-radius: 5px;
                text-align: center;
            }
            QProgressBar::chunk {
                background: #1a75ff;
                width: 20px;
                height: 20px;
            }
    )");  // Style the progress bar with custom colors and appearance
    layout->addWidget(progressBar);  // Add progress bar to the layout

    // Set layout for the loading screen widget
    loadingScreen->setLayout(layout);

    // Timer to simulate loading progress
    QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [progressBar, timer, loadingScreen, mainWindow]() {
        int value = progressBar->value();       // Get current value of the progress bar
        if (value < 100) {
            progressBar->setValue(value + 2);   // Increase progress by 2% on each timeout
        } else {
            timer->stop();                      // Stop the timer when progress reaches 100%
            loadingScreen->hide();              // Hide the loading screen
            mainWindow->show();                 // Show the main window
        }
    });

    timer->start(25);       // Start the timer with a 25ms interval for updating progress

    return loadingScreen;   // Return the created loading screen widget
}
