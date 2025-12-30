#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "FlightServer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ServerWindow; }
QT_END_NAMESPACE

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

    void appendLog(const QString &msg);

signals:
    void logSignal(const QString &msg);

public slots:
    void onLogReceived(const QString &msg);

private slots:
    // 服务器控制
    void on_btnStart_clicked();
    void on_btnPause_clicked();
    void on_btnRefresh_clicked();

    // 业务功能
    void on_btnDeleteFlight_clicked();
    void on_btnShowAddDialog_clicked();
    void on_btnCancelOrder_clicked();
    void on_btnShowAddOrderDialog_clicked();
    void on_btnDeleteUser_clicked();
    void on_btnShowAddUserDialog_clicked();

private:
    void initTables();
    void refreshOnlineUsers();
    void refreshFlights();
    void refreshAllUsers();
    void refreshOrders();
    void updateUIState();

    // 服务器控制
    void startServer();
    void stopServer(bool notifyClients = true);
    void restartServer();

private:
    Ui::ServerWindow *ui;
    FlightServer *m_server;
    bool m_isServerRunning = false;
};

#endif // SERVERWINDOW_H
