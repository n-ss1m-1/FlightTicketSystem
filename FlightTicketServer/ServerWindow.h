#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QTcpSocket> // 【必须加这行】
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

    void appendLog(const QString &msg);

signals:
    void logSignal(const QString &msg);

private slots:
    void onLogReceived(const QString &msg);

    // --- 按钮槽函数 ---
    void on_btnStart_clicked();
    void on_btnPause_clicked(); // 【必须声明这个】
    // 注意：on_btnStop_clicked 已经被删除了，不要写它

    void on_btnRefresh_clicked();

    // --- 业务功能 ---
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

private:
    Ui::ServerWindow *ui;
    FlightServer *m_server;

    // 【核心修复】必须在这里声明这个列表，否则cpp文件里会报错
    QList<QTcpSocket*> m_clientList;
};

#endif // SERVERWINDOW_H
