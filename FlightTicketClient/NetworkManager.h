#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include "Common/Models.h"

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager* instance();

    void connectToServer(const QString &host, quint16 port);
    bool isConnected() const;

    void sendJson(const QJsonObject &obj);

    void setServer(const QString& host, quint16 port);
    void reconnect(); // 重连

    void login(const QString& username, const QString& password); // 登录
    void registerUser(const QString& username, const QString& password,
                      const QString& phone, const QString& realName, const QString& idCard); // 注册
    void changePassword(const QString& username, const QString& oldPwd, const QString& newPwd); // 修改密码

    bool isLoggedIn() const;
    void setLoggedIn(bool loggedIn);

    // QJsonObject m_userJson;
    QString m_username = "";
    Common::UserInfo m_userInfo;

    void logout(); // 退出登录
    void clearSession(); // 清理登录态/缓存

signals:
    void connected();
    void disconnected();
    void jsonReceived(const QJsonObject &obj);
    void errorOccurred(const QString &msg);
    void notConnected(); // sendJson()未连接时触发弹窗
    void reconnectRequested();
    void loginStateChanged(bool loggedIn); // 登录状态改变

    void forceLogout(const QString& reason);   // 意外断连强制登出

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);

private:
    explicit NetworkManager(QObject *parent = nullptr);
    void processBuffer();

    QTcpSocket m_socket;
    QByteArray m_buffer;

    QString m_host;
    quint16 m_port = 0;

    bool m_loggedIn = false;

    void handleUnexpectedDisconnect(const QString& reason);

    bool m_manualDisconnect = false; // 主动断开标记（logout时置true）
    bool m_disconnectHandled = false;
};

#endif // NETWORKMANAGER_H
