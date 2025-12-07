#include "ClientHandler.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead,
            this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &ClientHandler::onDisconnected);
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
    QString type = obj.value("type").toString();
    QJsonObject data = obj.value("data").toObject();

    if (type == "login") {
        QString username = data.value("username").toString();
        QString password = data.value("password").toString();

        qInfo() << "Login request from user:" << username
                << "password:" << password;

        // 目前不接数据库，直接返回“成功”
        QJsonObject resp{
            {"type", "login_response"},
            {"success", true},
            {"message", "服务器已收到登录请求（未接数据库）"}
        };
        sendJson(resp);
    } else {
        // 其他类型暂不支持
        QJsonObject resp{
            {"type", "error"},
            {"message", "Unknown request type: " + type}
        };
        sendJson(resp);
    }
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
