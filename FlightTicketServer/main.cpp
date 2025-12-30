#include <QApplication>
#include "ServerWindow.h"
#include "DBManager.h"
#include <QInputDialog>
// 这里的密码改成你自己的！
static QString DBpassword = "你的密码";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ServerWindow w;
    w.show();
    return a.exec();
}
