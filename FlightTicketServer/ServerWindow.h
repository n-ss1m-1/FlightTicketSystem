#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include "FlightServer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ServerWindow; }
QT_END_NAMESPACE

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

    // 供日志拦截器调用
    static void appendLog(const QString& msg);

private slots:
    void on_btnStart_clicked();
    void on_btnStop_clicked();
    void on_btnRefresh_clicked();
    void onLogReceived(const QString& msg); // 接收日志信号
    void on_btnDeleteFlight_clicked();
    void on_btnShowAddDialog_clicked();
    void on_btnCancelOrder_clicked();
    void on_btnShowAddOrderDialog_clicked();
    void on_btnDeleteUser_clicked();
    void on_btnShowAddUserDialog_clicked();
signals:
    void logSignal(const QString& msg); // 发送日志信号

private:
    Ui::ServerWindow *ui;
    FlightServer *m_server;

    void initTables();
    void refreshOnlineUsers();
    void refreshFlights();
    void refreshAllUsers();
    void refreshOrders();
};
void on_btnShowAddDialog_clicked();
#endif // SERVERWINDOW_H
