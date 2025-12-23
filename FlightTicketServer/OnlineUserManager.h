#ifndef ONLINEUSERMANAGER_H
#define ONLINEUSERMANAGER_H

#include <QObject>
#include <QMap>
#include "Common/Models.h"

/*头文件循环包含问题：
 * 头文件中：前向声明---告知存在类,但是不能使用其成员
 * 源文件中：再包含相应头文件
*/
class ClientHandler;

class OnlineUserManager : public QObject
{
    Q_OBJECT
public:
    static OnlineUserManager& instance();   //单例模式

    //添加在线用户
    void addOnlineUser(ClientHandler* handler);
    //移除在线用户
    void removeOnlineUser(ClientHandler* handler);
    //根据连接查找用户信息
    Common::UserInfo getUserInfoByHandler(ClientHandler* Handler);
    //获取所有在线用户
    QList<Common::UserInfo> getAllUsers();

private:
    OnlineUserManager() = default;
    QMap<ClientHandler*, Common::UserInfo> m_users;  //连接->用户信息映射

};

#endif // ONLINEUSERMANAGER_H
