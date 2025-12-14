#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    static NetworkManager* instance();

    void connectToServer(const QString &host, quint16 port);
    bool isConnected() const;

    void sendJson(const QJsonObject &obj);

signals:
    void connected();
    void disconnected();
    void jsonReceived(const QJsonObject &obj);
    void errorOccurred(const QString &msg);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);

private:
    explicit NetworkManager(QObject *parent = nullptr);
    void processBuffer();

private:
    QTcpSocket m_socket;
    QByteArray m_buffer;
};

#endif // NETWORKMANAGER_H
