#include "OrdersPage.h"
#include "ui_OrdersPage.h"

OrdersPage::OrdersPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OrdersPage)
{
    ui->setupUi(this);
}

OrdersPage::~OrdersPage()
{
    delete ui;
}
