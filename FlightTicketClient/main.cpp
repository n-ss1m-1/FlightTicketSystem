#include <QApplication>
#include "MainWindow.h"
#include "SettingsManager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("FlightTicketClient");

    SettingsManager::instance().load();
    SettingsManager::instance().applyAll();

    MainWindow w;
    w.show();
    return a.exec();
}
