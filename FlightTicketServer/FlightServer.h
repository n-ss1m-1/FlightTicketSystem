#ifndef FLIGHTSERVER_H
#define FLIGHTSERVER_H

#include <QObject>
#include <QTcpServer>

class FlightServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit FlightServer(QObject *parent = nullptr);
    bool start(quint16 port);

private slots:
    void onNewConnection();

private:
    QTcpServer m_server;

};

#endif // FLIGHTSERVER_H
