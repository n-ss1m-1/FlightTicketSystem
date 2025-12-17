#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "pages/FlightsPage.h"
#include "pages/OrdersPage.h"
#include "pages/ProfilePage.h"
#include "pages/HomePage.h"
#include <QTabBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tabWidget->clear();
}

MainWindow::~MainWindow()
{
    delete ui;
}
