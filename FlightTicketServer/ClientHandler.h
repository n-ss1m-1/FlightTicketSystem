#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processBuffer();
    void handleJson(const QJsonObject &obj);
    void sendJson(const QJsonObject &obj);

private:
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
};

#endif // CLIENTHANDLER_H
