QT       += core gui network printsupport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Database/data_buffer.cpp \
    Database/database_manager.cpp \
    UDP/udpreceiver.cpp \
    UDP/udpsender.cpp \
    Glowplug/glowplug.cpp \
    Igniter/igniter_cycle.cpp \
    main.cpp \
    UDP/config.cpp \
    Cooling/cooling_cycle.cpp \
    Engine/engine_cycle.cpp \
    tools/loading_screen.cpp \
    mainwindow.cpp \
    Test/test.cpp \
    tools/scaler.cpp \
    valve/valve.cpp \
    Graphic/qcustomplot.cpp \
    Graphic/graphic.cpp \
    Extraction/extraction_cycle.cpp


HEADERS += \
    Database/data_buffer.h \
    Database/database_manager.h \
    UDP/udpreceiver.h \
    UDP/udpsender.h \
    Glowplug/glowplug.h \
    Igniter/igniter_cycle.h \
    tools/chrono.h \
    UDP/config.h \
    Cooling/cooling_cycle.h \
    Engine/engine_cycle.h \
    UDP/global_variable.h \
    tools/loading_screen.h \
    tools/loghelper.h \
    tools/logo.h \
    mainwindow.h \
    Test/test.h \
    tools/scaler.h \
    valve/valve.h \
    tools/closeButton.h \
    Graphic/qcustomplot.h \
    Graphic/graphic.h \
    Extraction/extraction_cycle.h


RESOURCES += pictures.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target