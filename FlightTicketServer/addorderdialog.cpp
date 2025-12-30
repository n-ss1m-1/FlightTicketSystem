#include "AddOrderDialog.h"
#include "ui_AddOrderDialog.h"
#include "DBManager.h"
#include <QMessageBox>

AddOrderDialog::AddOrderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddOrderDialog)
{
    ui->setupUi(this);
    setWindowTitle("补录新订单");
}

AddOrderDialog::~AddOrderDialog()
{
    delete ui;
}

void AddOrderDialog::on_btnCancel_clicked()
{
    reject();
}

void AddOrderDialog::on_btnConfirm_clicked()
{
    // 1. 获取输入
    qint64 userId = ui->editUserId->text().toLongLong();
    qint64 flightId = ui->editFlightId->text().toLongLong();
    QString name = ui->editPassengerName->text().trimmed();
    QString idCard = ui->editPassengerIdCard->text().trimmed();

    // 2. 校验
    if (userId <= 0 || flightId <= 0 || name.isEmpty() || idCard.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写完整信息，ID必须是数字");
        return;
    }

    // 3. 组装 OrderInfo 对象
    Common::OrderInfo order;
    order.userId = userId;
    order.flightId = flightId;
    order.passengerName = name;
    order.passengerIdCard = idCard;
    // 价格、座位号、状态会自动生成，不用填
    int status = ui->comboStatus->currentIndex(); // 或者 currentText().toInt()

    // 修改 SQL
    QList<QVariant> params;
    QString sql = "INSERT INTO orders (..., status) VALUES (..., ?)";
    params << '... '<< status;
    // 4. 调用 DBManager 现成的接口
    QString err;
    DBResult ret = DBManager::instance().createOrder(order, &err);

    if (ret == DBResult::Success) {
        QMessageBox::information(this, "成功",
                                 QString("下单成功！\n订单号：%1\n座位号：%2\n价格：%3")
                                     .arg(order.id).arg(order.seatNum).arg(order.priceCents));
        accept();
    } else {
        QMessageBox::critical(this, "失败", "下单失败: " + err);
    }
}
