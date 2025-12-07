#ifndef FLIGHTSERVER_H
#define FLIGHTSERVER_H

#endif // FLIGHTSERVER_H
#pragma once

#include <QObject>
#include <QTcpServer>

class FlightServer : public QObject
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
