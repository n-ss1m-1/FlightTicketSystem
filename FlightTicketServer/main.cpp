#include <QCoreApplication>
#include "FlightServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    FlightServer server;
    if (!server.start(12345)) return -1;

    return a.exec();
}
