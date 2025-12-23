#include <QCoreApplication>
#include "FlightServer.h"
#include "DBManager.h"

static QString DBpassword = "passwd"; // 数据库密码

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //初始化数据库连接
    QString errMsg;                              //此处使用你自己设置的数据库用户名和密码
    bool Connected=DBManager::instance().connect("127.0.0.1",3306,"root",DBpassword,"flight_ticket",&errMsg);
    if(Connected) qInfo()<<"数据库连接成功";
    else
    {
        qCritical()<<"数据库连接失败(请查看main.cpp): "<<errMsg;
        return -1;
    }

    //启动服务器监听
    FlightServer server;
    if (!server.start(12345)) return -1;

    return a.exec();
}
