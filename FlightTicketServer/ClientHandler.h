#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include "Common/Models.h"

class OnlineUserManager;

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);
    const Common::UserInfo& getUserInfo() const {return m_userInfo;}            //获取该连接的用户信息
    void setUserInfo(const Common::UserInfo& user) {m_userInfo=user;}           //维护登陆的用户信息
    bool isLoggedIn() const {return isLogin;}                                   //检查用户是否真正登陆 避免非法JSON构造
    QTcpSocket* getSocket() const {return m_socket;};                           //返回socket

    void processBuffer();
    void handleJson(const QJsonObject &obj);
    void sendJson(const QJsonObject &obj);

signals:
    void loginSuccess();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    Common::UserInfo m_userInfo;    //保存连接的用户信息
    bool isLogin;
};

#endif // CLIENTHANDLER_H
