#include <QPoint>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

int screenW = 1920;
int screenH = 1080;

int refWidth  = 1533;
int refHeight = 767;

void setScreenSize(QWidget  *widget){
    // Retrieving the dimensions of the size currently in use
    QScreen *screen = widget->screen();
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    QSize screenSize = screen->size();
    screenW = screenSize.width();
    screenH = screenSize.height();
};

// Scales a QPoint defined in a "reference space"
// into the coordinates of the current screen
int scale_x(int x)
{
    // scaling factor
    double scaleX = static_cast<double>(screenW) / refWidth;

    // Applies scaling
    x = static_cast<int>(x * scaleX);

    return x;
}

int scale_y(int y)
{
    // Scaling factors
    double scaleY = static_cast<double>(screenH) / refHeight;

    // applies scaling
    y = static_cast<int>(y * scaleY);

    return y;
}
