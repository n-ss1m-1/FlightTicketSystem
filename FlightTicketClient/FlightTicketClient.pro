QT       += core gui widgets network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MainWindow.cpp \
    NetworkManager.cpp \
    SettingsManager.cpp \
    main.cpp \
    pages/FlightsPage/FlightsPage.cpp \
    pages/FlightsPage/PassengerPickDialog.cpp \
    pages/HomePage/HomePage.cpp \
    pages/OrdersPage/OrdersPage.cpp \
    pages/ProfilePage/ChangePhoneDialog.cpp \
    pages/ProfilePage/ChangePwdDialog.cpp \
    pages/ProfilePage/LoginDialog.cpp \
    pages/ProfilePage/ProfilePage.cpp \
    pages/ProfilePage/RegisterDialog.cpp

HEADERS += \
    MainWindow.h \
    NetworkManager.h \
    SettingsManager.h \
    pages/FlightsPage/FlightsPage.h \
    pages/FlightsPage/PassengerPickDialog.h \
    pages/HomePage/HomePage.h \
    pages/OrdersPage/OrdersPage.h \
    pages/ProfilePage/ChangePhoneDialog.h \
    pages/ProfilePage/ChangePwdDialog.h \
    pages/ProfilePage/LoginDialog.h \
    pages/ProfilePage/ProfilePage.h \
    pages/ProfilePage/RegisterDialog.h

FORMS += \
    MainWindow.ui \
    pages/FlightsPage/FlightsPage.ui \
    pages/FlightsPage/PassengerPickDialog.ui \
    pages/HomePage/HomePage.ui \
    pages/OrdersPage/OrdersPage.ui \
    pages/ProfilePage/ChangePhoneDialog.ui \
    pages/ProfilePage/ChangePwdDialog.ui \
    pages/ProfilePage/LoginDialog.ui \
    pages/ProfilePage/ProfilePage.ui \
    pages/ProfilePage/RegisterDialog.ui

INCLUDEPATH += $$PWD/..

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
