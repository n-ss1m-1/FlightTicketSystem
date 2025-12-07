#include "FlightServer.h"
#include "ClientHandler.h"
#include <QTcpSocket>
#include <QDebug>

FlightServer::FlightServer(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection,
            this, &FlightServer::onNewConnection);
}

bool FlightServer::start(quint16 port)
{
    if (!m_server.listen(QHostAddress::Any, port)) {
        qCritical() << "Server listen failed:" << m_server.errorString();
        return false;
    }
    qInfo() << "Server listening on port" << port;
    return true;
}

void FlightServer::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        qInfo() << "New client from" << socket->peerAddress().toString();

        // 交给 ClientHandler 管理
        new ClientHandler(socket, this);   // handler 自己在断开时销毁
    }
}
