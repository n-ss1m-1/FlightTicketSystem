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
    connect(&m_socket, &QTcpSocket::readyRead,
            this, &NetworkManager::onReadyRead);
    connect(&m_socket, &QTcpSocket::connected,
            this, &NetworkManager::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected,
            this, &NetworkManager::onDisconnected);
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
        qWarning() << "Not connected, cannot send";
        return;
    }
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');
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

void NetworkManager::onConnected()
{
    emit connected();
}

void NetworkManager::onDisconnected()
{
    emit disconnected();
}

void NetworkManager::onError(QAbstractSocket::SocketError)
{
    emit errorOccurred(m_socket.errorString());
}
