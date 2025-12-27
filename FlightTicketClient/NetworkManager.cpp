#include "NetworkManager.h"
#include "Common/Protocol.h"
#include <QJsonDocument>
#include <QDebug>
#include <QMessageBox>

NetworkManager* NetworkManager::instance()
{
    static NetworkManager inst;
    return &inst;
}

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(&m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &NetworkManager::onError);
}

void NetworkManager::connectToServer(const QString &host, quint16 port)
{
    setServer(host, port);
    if (isConnected()) return;
    m_socket.connectToHost(host, port);
}

bool NetworkManager::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendJson(const QJsonObject &obj)
{
    if (!isConnected()) {
        qWarning() << "未连接，无法发送";
        if (isLoggedIn()) {
            handleUnexpectedDisconnect("连接已断开，已自动退出登录");
        } else {
            emit notConnected();
        }
        return;
    }
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n'); // 每条消息以 \n 结尾
    m_socket.write(data);
}

void NetworkManager::onReadyRead()
{
    m_buffer.append(m_socket.readAll());
    processBuffer();
}

void NetworkManager::processBuffer()
{
    while (true) {
        int idx = m_buffer.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            emit jsonReceived(doc.object());
        } else {
            qWarning() << "JSON parse error:" << err.errorString();
        }
    }
}

void NetworkManager::setServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
}

void NetworkManager::reconnect()
{
    if (isConnected()) return;
    if (m_host.isEmpty() || m_port == 0) {
        qWarning() << "Server host/port not set";
        return;
    }
    m_socket.abort();
    m_socket.connectToHost(m_host, m_port);
}

void NetworkManager::login(const QString &username, const QString &password)
{
    QJsonObject data;
    data.insert("username", username);
    data.insert("password", password);

    QJsonObject req;
    req.insert(Protocol::KEY_TYPE, Protocol::TYPE_LOGIN);
    req.insert(Protocol::KEY_DATA, data);

    sendJson(req);
}

void NetworkManager::registerUser(const QString &username, const QString &password,
                                  const QString &phone, const QString &realName, const QString &idCard)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;
    data["phone"] = phone;
    data["realName"] = realName;
    data["idCard"] = idCard;

    QJsonObject req;
    req[Protocol::KEY_TYPE] = Protocol::TYPE_REGISTER;
    req[Protocol::KEY_DATA] = data;

    sendJson(req);
}

void NetworkManager::changePassword(const QString &username, const QString &oldPwd, const QString &newPwd)
{
    QJsonObject data;
    data["username"] = username;
    data["oldPassword"] = oldPwd;
    data["newPassword"] = newPwd;

    QJsonObject req;
    req[Protocol::KEY_TYPE] = Protocol::TYPE_CHANGE_PWD;
    req[Protocol::KEY_DATA] = data;

    sendJson(req);
}

void NetworkManager::onConnected() {
    m_disconnectHandled = false;
    emit connected();
}

void NetworkManager::onDisconnected() {
    if (isLoggedIn()) {
        handleUnexpectedDisconnect("与服务器断开连接，已自动退出登录");
    } else {
        emit disconnected();
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError err) {
    if (err == QAbstractSocket::RemoteHostClosedError) {
        return; // 避免和disconnected()重复
    }

    // 如果已经登录，网络错误就强制登出
    if (isLoggedIn()) {
        handleUnexpectedDisconnect("网络错误：" + m_socket.errorString() + "，已自动退出登录");
    } else {
        emit errorOccurred(m_socket.errorString());
    }
}

bool NetworkManager::isLoggedIn() const
{
    return m_loggedIn;
}

void NetworkManager::setLoggedIn(bool loggedIn)
{
    if (m_loggedIn == loggedIn)
        return;

    m_loggedIn = loggedIn;
    emit loginStateChanged(m_loggedIn);
}

void NetworkManager::clearSession()
{
    m_buffer.clear();
    setLoggedIn(false);

    m_username.clear();
    m_userInfo = {};
}

void NetworkManager::handleUnexpectedDisconnect(const QString& reason)
{
    if (m_disconnectHandled) return;
    m_disconnectHandled = true;

    // 主动退出不触发强制登出提示
    if (m_manualDisconnect) {
        m_manualDisconnect = false;
        clearSession();
        emit disconnected();
        return;
    }

    // 意外断连：清理登录态，通知UI强制登出
    clearSession();
    emit forceLogout(reason);
    emit disconnected();
}

void NetworkManager::logout()
{
    if (isConnected()) {
        QJsonObject req;
        req.insert(Protocol::KEY_TYPE, Protocol::TYPE_LOGOUT);

        QJsonObject data;
        data.insert("username", m_username);
        req.insert(Protocol::KEY_DATA, data);

        sendJson(req);
    }

    clearSession();
}
