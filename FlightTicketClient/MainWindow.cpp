#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "pages/FlightsPage/FlightsPage.h"
#include "pages/HomePage/HomePage.h"
#include "pages/OrdersPage/OrdersPage.h"
#include "pages/ProfilePage/ProfilePage.h"
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

    auto bindPage = [&](QWidget* tabContainer, QWidget* page){
        if (!tabContainer->layout())
            tabContainer->setLayout(new QVBoxLayout(tabContainer));
        tabContainer->layout()->setContentsMargins(0,0,0,0);
        tabContainer->layout()->addWidget(page);
    };

    HomePage* homePage = new HomePage(this);
    FlightsPage* flightsPage = new FlightsPage(this);
    OrdersPage* ordersPage = new OrdersPage(this);
    ProfilePage* profilePage = new ProfilePage(this);

    // 绑定到容器
    bindPage(ui->tabHome, homePage);
    bindPage(ui->tabFlights, flightsPage);
    bindPage(ui->tabOrders, ordersPage);
    bindPage(ui->tabProfile, profilePage);

    connect(homePage, &HomePage::requestGoFlights, [this](){
        ui->tabWidget->setCurrentIndex(1);
    });

    connect(homePage, &HomePage::requestGoOrders, [this, ordersPage](){
        ordersPage->loadOrders();
        ui->tabWidget->setCurrentIndex(2);
    });

    connect(homePage, &HomePage::requestGoProfile, [this](){
        ui->tabWidget->setCurrentIndex(3);
    });

    // 网络
    auto nm = NetworkManager::instance();
    connect(nm, &NetworkManager::connected, this, []{ qDebug() << "服务器已连接"; });
    connect(nm, &NetworkManager::disconnected, this, [this]{
        QMessageBox::warning(this, "网络断开", "与服务器连接已断开");
    });

    nm->connectToServer(kHost, kPort);
}

MainWindow::~MainWindow()
{
    delete ui;
}
