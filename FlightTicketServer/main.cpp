#include <QApplication>
#include "ServerWindow.h"
#include "DBManager.h"
#include <QInputDialog>
// 这里的密码改成你自己的！
static QString DBpassword = "你的密码";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 需求2：手动输入数据库密码
    bool ok;
    QString text = QInputDialog::getText(nullptr, "安全验证",
                                         "请输入数据库密码:", QLineEdit::Password,
                                         "", &ok);
    // 如果点了取消，或者密码为空(看你是否允许空)，直接退出
    if (!ok) {
        return 0;
    }

    // 这里的 text 就是用户输入的密码
    // 把下面连接数据库的 "root" 改成 text
    if (!DBManager::instance().connect("127.0.0.1", 3306, "root", text, "flight_ticket")) {
        // ... 报错代码 ...
    }

    ServerWindow w;
    w.show();
    return a.exec();
}
