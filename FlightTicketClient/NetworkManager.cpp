#include "NetworkManager.h"
#include <QJsonDocument>
#include <QDebug>

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

void NetworkManager::onConnected() { emit connected(); }
void NetworkManager::onDisconnected() { emit disconnected(); }
void NetworkManager::onError(QAbstractSocket::SocketError) { emit errorOccurred(m_socket.errorString()); }
