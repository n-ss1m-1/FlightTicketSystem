#include "FlightServer.h"
#include "ClientHandler.h"
#include "OnlineUserManager.h"
#include <QTcpSocket>
#include <QDebug>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QThread>  // 添加QThread头文件
#include <QEventLoop>

FlightServer::FlightServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    // 连接新连接信号
    connect(m_server, &QTcpServer::newConnection, this, &FlightServer::onNewConnection);

    qInfo() << "FlightServer created";
}

FlightServer::~FlightServer()
{
    // 清理所有客户端连接
    disconnectAllClients();

    // 停止服务器
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }

    qInfo() << "FlightServer destroyed";
}

bool FlightServer::start(quint16 port)
{
    if (!m_server) {
        qCritical() << "Server pointer is null!";
        return false;
    }

    // 如果已经在监听，先停止
    if (m_server->isListening()) {
        qInfo() << "Server is already running, stopping first...";
        stop();
        QThread::msleep(100);  // 短暂延迟
    }

    // 尝试监听端口
    if (m_server->listen(QHostAddress::Any, port)) {
        m_currentPort = port;
        m_paused = false;

        // 获取本机IP地址
        QString ipAddress;
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        for (const QHostAddress &address : ipAddressesList) {
            if (address != QHostAddress::LocalHost && address.toIPv4Address()) {
                ipAddress = address.toString();
                break;
            }
        }
        if (ipAddress.isEmpty())
            ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

        qInfo() << "===============================================";
        qInfo() << "Server started successfully!";
        qInfo() << "Listening on: " << ipAddress << ":" << port;
        qInfo() << "===============================================";

        emit serverStarted();
        return true;
    } else {
        QString errorMsg = QString("Failed to start server on port %1: %2")
        .arg(port)
            .arg(m_server->errorString());
        qCritical() << errorMsg;

        // 尝试使用其他端口
        for (quint16 altPort = port + 1; altPort <= port + 10; ++altPort) {
            if (m_server->listen(QHostAddress::Any, altPort)) {
                m_currentPort = altPort;
                m_paused = false;

                qWarning() << "Using alternative port:" << altPort;
                emit serverStarted();
                return true;
            }
        }

        return false;
    }
}

void FlightServer::stop()
{
    if (m_server && m_server->isListening()) {
        // 先向所有客户端发送服务器关闭通知
        notifyAllClientsBeforeShutdown("SERVER_STOP");

        // 短暂延迟，让客户端有时间处理消息
        QEventLoop loop;
        QTimer::singleShot(500, &loop, &QEventLoop::quit);
        loop.exec();

        // 断开所有客户端连接
        disconnectAllClients();

        // 停止服务器监听
        m_server->close();
        m_paused = false;

        qInfo() << "===============================================";
        qInfo() << "Server stopped";
        qInfo() << "===============================================";

        emit serverStopped();
    }
}

void FlightServer::pause()
{
    if (!m_server || !m_server->isListening() || m_paused) {
        return;
    }

    m_paused = true;

    // 向所有客户端发送暂停通知
    notifyAllClientsBeforeShutdown("SERVER_PAUSE");

    // 短暂延迟后断开所有客户端连接
    QTimer::singleShot(1000, this, [this]() {
        disconnectAllClients();
        qInfo() << "===============================================";
        qInfo() << "Server paused";
        qInfo() << "All client connections have been closed";
        qInfo() << "===============================================";

        emit serverPaused();
    });
}

void FlightServer::resume()
{
    if (!m_server || !m_server->isListening() || !m_paused) {
        return;
    }

    m_paused = false;

    qInfo() << "===============================================";
    qInfo() << "Server resumed";
    qInfo() << "Now accepting new connections";
    qInfo() << "===============================================";

    emit serverResumed();
}

bool FlightServer::isRunning() const
{
    return m_server && m_server->isListening();
}

bool FlightServer::isPaused() const
{
    return m_paused;
}

int FlightServer::clientCount() const
{
    return m_clientHandlers.size();
}

void FlightServer::onNewConnection()
{
    if (!m_server) {
        return;
    }

    // 如果服务器暂停，拒绝新连接
    if (m_paused && m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (socket) {
            // 发送服务器暂停消息
            QJsonObject pauseMsg;
            pauseMsg["type"] = "server_status";
            pauseMsg["status"] = "paused";
            pauseMsg["message"] = "服务器维护中，请稍后重试";
            pauseMsg["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

            QByteArray data = QJsonDocument(pauseMsg).toJson(QJsonDocument::Compact);
            socket->write(data);
            socket->flush();
            socket->waitForBytesWritten(1000);
            socket->disconnectFromHost();
            socket->deleteLater();

            qInfo() << "Rejected new connection from" << socket->peerAddress().toString()
                    << "while server is paused";
        }
        return;
    }

    // 正常处理新连接
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();

        if (!socket) {
            continue;
        }

        QString clientInfo = QString("%1:%2").arg(socket->peerAddress().toString())
                                 .arg(socket->peerPort());

        qInfo() << "New client connected:" << clientInfo;

        // 保存socket引用
        m_clientSockets.append(socket);

        // 创建客户端处理器
        ClientHandler* handler = new ClientHandler(socket, this);
        m_clientHandlers.append(handler);

        // 连接断开信号
        connect(socket, &QTcpSocket::disconnected, this, [this, socket, clientInfo, handler]() {
            m_clientSockets.removeOne(socket);
            m_clientHandlers.removeOne(handler);

            // 从在线用户管理器中移除
            if (handler->isLoggedIn()) {
                OnlineUserManager::instance().removeOnlineUser(handler);
            }

            qInfo() << "Client disconnected:" << clientInfo;
            emit clientDisconnected(clientInfo);

            socket->deleteLater();
            handler->deleteLater();
        });

        // 连接登录成功信号
        connect(handler, &ClientHandler::loginSuccess, this, [handler]() {
            OnlineUserManager::instance().addOnlineUser(handler);
        });

        emit clientConnected(clientInfo);
    }
}

void FlightServer::notifyAllClientsBeforeShutdown(const QString& messageType)
{
    if (m_clientHandlers.isEmpty()) {
        return;
    }

    QString message;
    QString status;

    if (messageType == "SERVER_PAUSE") {
        message = "服务器维护中，请稍后重试";
        status = "paused";
    } else {
        message = "服务器即将关闭，请保存您的工作";
        status = "stopping";
    }

    int notifiedCount = 0;

    for (ClientHandler* handler : m_clientHandlers) {
        if (handler) {
            // 通过ClientHandler的公共接口发送消息
            QJsonObject serverMsg;
            serverMsg["type"] = "server_status";
            serverMsg["status"] = status;
            serverMsg["message"] = message;
            serverMsg["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

            // 假设ClientHandler有sendMessage或类似的方法
            handler->sendJson(serverMsg);
            notifiedCount++;
        }
    }

    qInfo() << "Notified" << notifiedCount << "clients about server"
            << (messageType == "SERVER_PAUSE" ? "pause" : "stop");
}

void FlightServer::disconnectAllClients()
{
    qInfo() << "Disconnecting all clients, count:" << m_clientHandlers.size();

    // 复制列表，因为断开连接时会从原列表中移除
    QList<ClientHandler*> handlers = m_clientHandlers;
    QList<QTcpSocket*> sockets = m_clientSockets;

    for (QTcpSocket* socket : sockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->disconnectFromHost();
        }
    }

    // 清理列表
    for (ClientHandler* handler : handlers) {
        if (handler && handler->isLoggedIn()) {
            OnlineUserManager::instance().removeOnlineUser(handler);
        }
        handler->deleteLater();
    }

    m_clientHandlers.clear();
    m_clientSockets.clear();

    qInfo() << "All clients disconnected";
}
