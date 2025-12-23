#ifndef ONLINEUSERMANAGER_H
#define ONLINEUSERMANAGER_H

#endif // ONLINEUSERMANAGER_H

#include <QObject>
#include <QMap>
#include "ClientHandler.h"
#include "Common/Models.h"

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
