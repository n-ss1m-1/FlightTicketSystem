#include "OnlineUserManager.h"
#include "ClientHandler.h"

OnlineUserManager& OnlineUserManager::instance()
{
    static OnlineUserManager inst;
    return inst;
}

void OnlineUserManager::addOnlineUser(ClientHandler* handler)
{
    if(handler && handler->isLoggedIn())
        m_users[handler]=handler->getUserInfo();
}

void OnlineUserManager::removeOnlineUser(ClientHandler* handler)
{
    m_users.remove(handler);
}

Common::UserInfo OnlineUserManager::getUserInfoByHandler(ClientHandler* Handler)
{
    return m_users[Handler];
}

QList<Common::UserInfo> OnlineUserManager::getAllUsers()
{
    return m_users.values();
}
