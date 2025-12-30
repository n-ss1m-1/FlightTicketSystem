#include <QApplication>
#include "ServerWindow.h"
#include "DBManager.h"
#include <QInputDialog>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ServerWindow w;
    w.show();
    return a.exec();
}
