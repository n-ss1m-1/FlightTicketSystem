#include "FlightServer.h"
#include "ClientHandler.h"
#include "OnlineUserManager.h"
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
        ClientHandler* handler = new ClientHandler(socket, this); // 断开时自销毁

        //登陆成功 注册到在线用户管理表中
        connect(handler, &ClientHandler::loginSuccess, [handler](){
            OnlineUserManager::instance().addOnlineUser(handler);
        });
    }
}


