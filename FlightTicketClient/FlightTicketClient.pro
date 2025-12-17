QT       += core gui widgets network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    LoginWindow.cpp \
    MainWindow.cpp \
    NetworkManager.cpp \
    main.cpp \
    pages/FlightsPage.cpp \
    pages/HomePage.cpp \
    pages/OrdersPage.cpp \
    pages/ProfilePage.cpp

HEADERS += \
    LoginWindow.h \
    MainWindow.h \
    NetworkManager.h \
    pages/FlightsPage.h \
    pages/HomePage.h \
    pages/OrdersPage.h \
    pages/ProfilePage.h

FORMS += \
    MainWindow.ui \
    pages/FlightsPage.ui \
    pages/HomePage.ui \
    pages/OrdersPage.ui \
    pages/ProfilePage.ui

INCLUDEPATH += $$PWD/..

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
