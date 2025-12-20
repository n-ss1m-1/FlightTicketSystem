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
    pages/FlightsPage/FlightsPage.cpp \
    pages/HomePage/HomePage.cpp \
    pages/OrdersPage/OrdersPage.cpp \
    pages/ProfilePage/ProfilePage.cpp

HEADERS += \
    LoginWindow.h \
    MainWindow.h \
    NetworkManager.h \
    pages/FlightsPage/FlightsPage.h \
    pages/HomePage/HomePage.h \
    pages/OrdersPage/OrdersPage.h \
    pages/ProfilePage/ProfilePage.h

FORMS += \
    MainWindow.ui \
    pages/FlightsPage/FlightsPage.ui \
    pages/HomePage/HomePage.ui \
    pages/OrdersPage/OrdersPage.ui \
    pages/ProfilePage/ProfilePage.ui

INCLUDEPATH += $$PWD/..

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
