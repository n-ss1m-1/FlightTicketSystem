#include "OrderDetailDialog.h"
#include "ui_OrderDetailDialog.h"

OrderDetailDialog::OrderDetailDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderDetailDialog)
{
    ui->setupUi(this);
    setWindowTitle("订单详情");
    setModal(true);
}

OrderDetailDialog::~OrderDetailDialog()
{
    delete ui;
}

QString OrderDetailDialog::statusToText(Common::OrderStatus s)
{
    switch (s) {
    case Common::OrderStatus::Booked:   return "已预订";
    case Common::OrderStatus::Canceled: return "已退票";
    case Common::OrderStatus::Finished: return "已完成";
    default: return "未知";
    }
}

QString OrderDetailDialog::dtToText(const QDateTime &dt)
{
    if (!dt.isValid()) return "--";
    return dt.toString("yyyy-MM-dd HH:mm");
}

void OrderDetailDialog::setData(const Common::OrderInfo &ord,
                                const Common::FlightInfo &flt,
                                const QString &sourceText)
{
    // 订单信息
    ui->lblOrderId->setText(QString::number(ord.id));
    ui->lblSource->setText(sourceText);
    ui->lblCreatedTime->setText(dtToText(ord.createdTime));
    ui->lblStatus->setText(statusToText(ord.status));
    ui->lblPrice->setText(QString("￥%1").arg(QString::number(ord.priceCents / 100.0, 'f', 2)));

    // 乘机人信息
    ui->lblPassengerName->setText(ord.passengerName.isEmpty() ? "--" : ord.passengerName);
    ui->lblPassengerIdCard->setText(ord.passengerIdCard.isEmpty() ? "--" : ord.passengerIdCard);
    ui->lblSeatNum->setText(ord.seatNum.isEmpty() ? "未分配" : ord.seatNum);

    // 航班信息
    ui->lblFlightNo->setText(flt.flightNo.isEmpty() ? "--" : flt.flightNo);
    ui->lblRoute->setText(QString("%1 → %2")
                              .arg(flt.fromCity.isEmpty() ? "--" : flt.fromCity)
                              .arg(flt.toCity.isEmpty() ? "--" : flt.toCity));
    ui->lblDepartTime->setText(dtToText(flt.departTime));
    ui->lblArriveTime->setText(dtToText(flt.arriveTime));
}
