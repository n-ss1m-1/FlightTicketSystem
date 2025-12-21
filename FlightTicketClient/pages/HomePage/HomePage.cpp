#include "HomePage.h"
#include "ui_HomePage.h"

HomePage::HomePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HomePage)
{
    ui->setupUi(this);

    // 检查按钮是否存在并连接信号
    if (ui->btnGoFlights) {
        connect(ui->btnGoFlights, &QPushButton::clicked, this, &HomePage::requestGoFlights);
    }
    if (ui->btnGoOrders) {
        connect(ui->btnGoOrders, &QPushButton::clicked, this, &HomePage::requestGoOrders);
    }
    if (ui->btnGoProfile) {
        connect(ui->btnGoProfile, &QPushButton::clicked, this, &HomePage::requestGoProfile);
    }
}

HomePage::~HomePage()
{
    delete ui;
}
