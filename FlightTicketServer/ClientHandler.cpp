#include "ClientHandler.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

#include "Common/Protocol.h"

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
}

void ClientHandler::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    processBuffer();
}

void ClientHandler::processBuffer()
{
    while (true) {
        int idx = m_buffer.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            handleJson(doc.object());
        } else {
            qWarning() << "JSON parse error from client:" << err.errorString();
        }
    }
}

void ClientHandler::handleJson(const QJsonObject &obj)
{
    const QString type = obj.value(Protocol::KEY_TYPE).toString();
    const QJsonObject data = obj.value(Protocol::KEY_DATA).toObject();

    if (type == Protocol::TYPE_LOGIN) {
        const QString username = data.value("username").toString();
        const QString password = data.value("password").toString();

        qInfo() << "Login request:" << username << password;

        // Demo：暂不接数据库，直接成功
        sendJson(Protocol::makeOkResponse(
            Protocol::TYPE_LOGIN_RESP,
            QJsonObject(),  // data 为空
            "服务器已收到登录请求（未接数据库）"
            ));
        return;
    }

    // 未知请求
    sendJson(Protocol::makeFailResponse(
        Protocol::TYPE_ERROR,
        "Unknown request type: " + type
        ));
}

void ClientHandler::sendJson(const QJsonObject &obj)
{
    if (!m_socket) return;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
}

void ClientHandler::onDisconnected()
{
    qInfo() << "Client disconnected";
    m_socket->deleteLater();
    deleteLater();
}
