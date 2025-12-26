QT       += core gui widgets network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ClientHandler.cpp \
    DBManager.cpp \
    FlightServer.cpp \
    OnlineUserManager.cpp \
    ServerWindow.cpp \
    addflightdialog.cpp \
    addorderdialog.cpp \
    adduserdialog.cpp \
    main.cpp

HEADERS += \
    ClientHandler.h \
    DBManager.h \
    FlightServer.h \
    OnlineUserManager.h \
    ServerWindow.h \
    addflightdialog.h \
    addorderdialog.h \
    adduserdialog.h

FORMS += \
    ServerWindow.ui \
    addflightdialog.ui \
    addorderdialog.ui \
    adduserdialog.ui

INCLUDEPATH += $$PWD/..

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
