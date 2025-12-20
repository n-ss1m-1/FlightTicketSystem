#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "pages/FlightsPage/FlightsPage.h"
#include "pages/HomePage/HomePage.h"
#include "pages/OrdersPage/OrdersPage.h"
#include "pages/ProfilePage/ProfilePage.h"
#include "NetworkManager.h"
#include <QMessageBox>

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

    bindPage(ui->tabHome, new HomePage(this));
    bindPage(ui->tabFlights, new FlightsPage(this));
    bindPage(ui->tabOrders, new OrdersPage(this));
    bindPage(ui->tabProfile, new ProfilePage(this));

    auto nm = NetworkManager::instance();

    connect(nm, &NetworkManager::connected, this, []{
        qDebug() << "Server connected";
    });
    connect(nm, &NetworkManager::disconnected, this, [this]{
        QMessageBox::warning(this, "网络断开", "与服务器连接已断开");
    });
    connect(nm, &NetworkManager::errorOccurred, this, [this](const QString& err){
        qDebug() << "Socket error:" << err;
    });

    nm->connectToServer(kHost, kPort);
}

MainWindow::~MainWindow()
{
    delete ui;
}
