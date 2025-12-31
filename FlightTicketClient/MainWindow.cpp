#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "NetworkManager.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDebug>

static const char* kHost = "127.0.0.1";
static const quint16 kPort = 12345;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("航班订票系统");

    auto bindPage = [&](QWidget* tabContainer, QWidget* page){
        if (!tabContainer->layout())
            tabContainer->setLayout(new QVBoxLayout(tabContainer));
        tabContainer->layout()->setContentsMargins(0,0,0,0);
        tabContainer->layout()->addWidget(page);
    };

    homePage = new HomePage(this);
    flightsPage = new FlightsPage(this);
    ordersPage = new OrdersPage(this);
    profilePage = new ProfilePage(this);
    homePage->m_profilePage = profilePage;
    flightsPage->m_profilePage = profilePage;

    // 绑定到容器
    bindPage(ui->tabHome, homePage);
    bindPage(ui->tabFlights, flightsPage);
    bindPage(ui->tabOrders, ordersPage);
    bindPage(ui->tabProfile, profilePage);

    connect(homePage, &HomePage::requestGoFlights, [this](){
        ui->tabWidget->setCurrentIndex(1);
    });

    connect(homePage, &HomePage::requestGoOrders, [this](){
        ordersPage->loadOrders();
        ui->tabWidget->setCurrentIndex(2);
    });

    connect(homePage, &HomePage::requestGoProfile, [this](){
        ui->tabWidget->setCurrentIndex(3);
    });

    connect(flightsPage, &FlightsPage::requestGoOrders, this, [this](qint64){
        ordersPage->loadOrders();
        ui->tabWidget->setCurrentIndex(2);
    });

    connect(NetworkManager::instance(),
            &NetworkManager::loginStateChanged,
            this,
            &MainWindow::updateTab);

    updateTab();

    // 网络
    auto nm = NetworkManager::instance();
    connect(nm, &NetworkManager::connected, this, [this]{
        qDebug() << "服务器已连接";

        if (m_reconnectPending) {
            m_reconnectPending = false;

            QMessageBox::information(this, "重连成功", "已重新连接到服务器。");
        }
    });

    // 未连接时弹窗重连
    auto showReconnectDialog = [this, nm](const QString& title, const QString& text) {
        if (m_reconnectDialogShowing) return;
        m_reconnectDialogShowing = true;

        QMessageBox box(this);
        box.setWindowTitle(title);
        box.setText(text);

        box.addButton("重连", QMessageBox::AcceptRole);
        box.addButton("取消", QMessageBox::RejectRole);

        box.exec();

        m_reconnectDialogShowing = false;

        if (box.buttonRole(box.clickedButton()) == QMessageBox::AcceptRole) {
            m_reconnectPending = true;
            nm->reconnect();
        }
    };

    connect(nm, &NetworkManager::notConnected, this, [=]{
        if (m_suppressNextReconnectDialog) { m_suppressNextReconnectDialog = false; return; }
        showReconnectDialog("未连接服务器", "当前未连接服务器。是否尝试重连？");
    });

    connect(nm, &NetworkManager::disconnected, this, [=]{
        if (m_suppressNextReconnectDialog) { m_suppressNextReconnectDialog = false; return; }
        showReconnectDialog("网络断开", "与服务器连接已断开。是否尝试重连？");
    });

    connect(nm, &NetworkManager::errorOccurred, this, [=](const QString& err){
        if (m_suppressNextReconnectDialog) { m_suppressNextReconnectDialog = false; return; }
        m_reconnectPending = false;
        showReconnectDialog("网络错误", "发生网络错误：\n" + err + "\n是否尝试重连？");
    });

    connect(nm, &NetworkManager::forceLogout, this, [=](const QString& reason){
        // forceLogout意外断连导致已登出
        m_reconnectPending = false;

        ui->tabWidget->setCurrentIndex(0);

        m_suppressNextReconnectDialog = true;

        QMessageBox::warning(this, "已退出登录", reason);
    });

    nm->connectToServer(kHost, kPort);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateTab()
{
    bool loggedIn = NetworkManager::instance()->isLoggedIn();

    ui->tabWidget->setTabEnabled(1, loggedIn); // Flights
    ui->tabWidget->setTabEnabled(2, loggedIn); // Orders

    int idx = ui->tabWidget->currentIndex();
    if (!loggedIn && (idx == 1 || idx == 2)) {
        ui->tabWidget->setCurrentIndex(0);
    }
}
