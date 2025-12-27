#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pages/FlightsPage/FlightsPage.h"
#include "pages/HomePage/HomePage.h"
#include "pages/OrdersPage/OrdersPage.h"
#include "pages/ProfilePage/ProfilePage.h"
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    bool m_reconnectDialogShowing = false; // 正在弹出网络重连窗口
    bool m_reconnectPending = false; // 正在重连
    bool m_suppressNextReconnectDialog = false; // forceLogout后抑制一次重连弹窗

    HomePage* homePage;
    FlightsPage* flightsPage;
    OrdersPage* ordersPage;
    ProfilePage* profilePage;

    void updateTab();
};

#endif // MAINWINDOW_H
