#include <QApplication>
#include "ServerWindow.h"
#include "DBManager.h"

// 这里的密码改成你自己的！
static QString DBpassword = "你的密码";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 初始化数据库连接
    // 这里的 "root" 和 DBpassword 根据你的实际情况修改
    if (!DBManager::instance().connect("127.0.0.1", 3306, "root", DBpassword, "flight_ticket")) {
        // 如果连不上，打印个错误，但在界面出来前可能看不到
    }

    ServerWindow w;
    w.show();

    return a.exec();
}
