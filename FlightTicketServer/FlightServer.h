#ifndef FLIGHTSERVER_H
#define FLIGHTSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QTimer>

class ClientHandler;
class QTcpSocket;

class FlightServer : public QObject
{
    Q_OBJECT

public:
    explicit FlightServer(QObject *parent = nullptr);
    ~FlightServer();

    // 服务器控制
    bool start(quint16 port = 12345);
    void stop();
    void pause();
    void resume();

    // 状态查询
    bool isRunning() const;
    bool isPaused() const;

    // 获取在线客户端数量
    int clientCount() const;

signals:
    void serverStarted();
    void serverStopped();
    void serverPaused();
    void serverResumed();
    void clientConnected(const QString& clientInfo);
    void clientDisconnected(const QString& clientInfo);
    void logMessage(const QString& message);

private slots:
    void onNewConnection();

private:
    void notifyAllClientsBeforeShutdown(const QString& messageType);
    void disconnectAllClients();

private:
    QTcpServer* m_server = nullptr;
    QList<QTcpSocket*> m_clientSockets;  // 保存客户端socket
    QList<ClientHandler*> m_clientHandlers;  // 保存客户端处理器
    bool m_paused = false;
    quint16 m_currentPort = 0;
};

#endif // FLIGHTSERVER_H
