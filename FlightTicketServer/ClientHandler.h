#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include "Common/Models.h"
#include "OnlineUserManager.h"

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);
    const Common::UserInfo& getUserInfo() const {return m_userInfo;}
    void setUserInfo(const Common::UserInfo& user) {m_userInfo=user;}
    bool isLoggedIn() const {return m_userInfo.id != 0;}

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processBuffer();
    void handleJson(const QJsonObject &obj);
    void sendJson(const QJsonObject &obj);

private:
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    Common::UserInfo m_userInfo;    //保存连接的用户信息
};

#endif // CLIENTHANDLER_H
