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
    setWindowTitle("添加新航班");
    // 默认时间设置为当前时间，方便调试
    ui->editTime->setDateTime(QDateTime::currentDateTime());
    ui->dateTimeArrive->setDateTime(QDateTime::currentDateTime().addSecs(7200)); // 默认2小时后
}

AddFlightDialog::~AddFlightDialog()
{
    delete ui;
}

void AddFlightDialog::on_btnCancel_clicked()
{
    this->reject();
}

void AddFlightDialog::on_btnConfirm_clicked()
{
    // 1. 获取输入
    QString flightNo = ui->editFlightNo->text().trimmed();
    QString fromCity = ui->editFrom->text().trimmed();
    QString toCity   = ui->editTo->text().trimmed();
    QDateTime departTime = ui->editTime->dateTime();
    QDateTime arriveTime = ui->dateTimeArrive->dateTime();

    // 价格和座位
    // 注意：数据库存的是“分”，界面输入的是“元”
    double priceYuan = ui->editPrice->text().toDouble();
    int priceCents = static_cast<int>(priceYuan * 100);
    int totalSeats = ui->editSeats->text().toInt();

    // 2. 基础校验
    if (flightNo.isEmpty() || fromCity.isEmpty() || toCity.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写完整信息");
        return;
    }
    if (arriveTime <= departTime) {
        QMessageBox::warning(this, "提示", "到达时间必须晚于出发时间");
        return;
    }

    // 3. 准备 SQL 语句
    // 修正点：显式写出所有字段名，去除多余符号
    // 根据你的schema，我们需要同时插入 seat_total 和 seat_left
    QString sql = "INSERT INTO flight "
                  "(flight_no, from_city, to_city, depart_time, arrive_time, price_cents, seat_total, seat_left) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    QList<QVariant> params;
    params << flightNo;
    params << fromCity;
    params << toCity;
    params << departTime;
    params << arriveTime;
    params << priceCents; // 存入 分
    params << totalSeats; // seat_total
    params << totalSeats; // seat_left (刚创建时，余票 = 总票)

    // 4. 执行
    QString err;
    int ret = DBManager::instance().update(sql, params, &err);

    if (ret > 0) {
        QMessageBox::information(this, "成功", "航班添加成功！");
        this->accept();
    } else {
        // 错误信息会在 err 中返回
        QMessageBox::critical(this, "错误", "添加失败: " + err);
    }
}
