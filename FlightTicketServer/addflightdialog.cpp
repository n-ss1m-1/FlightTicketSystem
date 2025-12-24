#include "AddFlightDialog.h"
#include "ui_AddFlightDialog.h"
#include "DBManager.h"
#include <QMessageBox>
#include <QDateTime>

AddFlightDialog::AddFlightDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddFlightDialog)
{
    ui->setupUi(this);
    setWindowTitle("添加新航班"); // 设置窗口标题
}

AddFlightDialog::~AddFlightDialog()
{
    delete ui;
}

void AddFlightDialog::on_btnCancel_clicked()
{
    this->reject(); // 关闭窗口，返回“取消”状态
}

void AddFlightDialog::on_btnConfirm_clicked()
{
    // 1. 获取输入
    QString flightNo = ui->editFlightNo->text().trimmed();
    QString fromCity = ui->editFrom->text().trimmed();
    QString toCity   = ui->editTo->text().trimmed();
    QDateTime departTime = ui->editTime->dateTime();
    int price = ui->editPrice->text().toInt();
    int totalSeats = ui->editSeats->text().toInt();

    // 2. 校验
    if (flightNo.isEmpty() || fromCity.isEmpty() || toCity.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写完整信息");
        return;
    }

    // 3. 计算到达时间 (+2.5小时)
    QDateTime arriveTime = departTime.addSecs(3600 * 2.5);

    // 4. 插入数据库
    QString sql = "INSERT INTO flight (flight_no, from_city, to_city, depart_time, arrive_time, price_cents, seat_total, seat_left, status) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0)";
    QList<QVariant> params;
    params << flightNo << fromCity << toCity << departTime << arriveTime << price << totalSeats << totalSeats;

    QString err;
    int ret = DBManager::instance().update(sql, params, &err);

    if (ret > 0) {
        QMessageBox::information(this, "成功", "添加成功！");
        this->accept(); // 关闭窗口，返回“成功”状态
    } else {
        QMessageBox::critical(this, "错误", "添加失败: " + err);
    }
}
