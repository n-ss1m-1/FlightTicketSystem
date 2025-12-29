#include "OrderDetailDialog.h"
#include "ui_OrderDetailDialog.h"

#include "NetworkManager.h"
#include "Common/Protocol.h"
#include <QJsonObject>
#include <QMessageBox>

OrderDetailDialog::OrderDetailDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderDetailDialog)
{
    ui->setupUi(this);
    setWindowTitle("订单详情");
    setModal(true);

    // 只在弹窗生命周期内监听网络回包（用于支付）
    m_conn = connect(NetworkManager::instance(), &NetworkManager::jsonReceived,
                     this, &OrderDetailDialog::onJsonReceived);
}

OrderDetailDialog::~OrderDetailDialog()
{
    if (m_conn) disconnect(m_conn);
    delete ui;
}

QString OrderDetailDialog::statusToText(Common::OrderStatus s)
{
    switch (s) {
    case Common::OrderStatus::Booked:   return "已预订";
    case Common::OrderStatus::Paid:     return "已支付";
    case Common::OrderStatus::Rescheduled: return "已改签";
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

void OrderDetailDialog::refreshPayButton()
{
    // 已支付：禁用并显示“已支付”
    if (m_ord.status == Common::OrderStatus::Paid) {
        ui->btnPay->setEnabled(false);
        ui->btnPay->setText("已支付");
        return;
    }

    // 未支付：允许支付（仅 Booked 状态）
    if (m_ord.status == Common::OrderStatus::Booked) {
        ui->btnPay->setEnabled(!m_waitingPayResp);
        ui->btnPay->setText(m_waitingPayResp ? "支付中..." : "支付");
        return;
    }

    // 其它状态：不可支付
    ui->btnPay->setEnabled(false);
    ui->btnPay->setText("不可支付");
}

void OrderDetailDialog::setData(const Common::OrderInfo &ord,
                                const Common::FlightInfo &flt,
                                const QString &sourceText)
{
    // 保存数据，后续支付/更新状态用
    m_ord = ord;
    m_flt = flt;
    m_sourceText = sourceText;

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

    // 根据状态更新按钮
    m_waitingPayResp = false;
    m_waitingCancelResp = false;
    refreshPayButton();
    refreshCancelButton();
}

void OrderDetailDialog::on_btnPay_clicked()
{
    // 二次保护
    if (m_ord.status == Common::OrderStatus::Paid) {
        refreshPayButton();
        return;
    }
    if (m_ord.status != Common::OrderStatus::Booked) {
        QMessageBox::warning(this, "提示", "该订单当前状态不可支付。");
        refreshPayButton();
        return;
    }
    if (NetworkManager::instance()->m_username.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录，无法支付。");
        refreshPayButton();
        return;
    }

    m_waitingPayResp = true;
    refreshPayButton();

    QJsonObject data;
    data.insert("orderId", static_cast<qint64>(m_ord.id));

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_PAY);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

void OrderDetailDialog::on_btnExit_clicked()
{
    reject();
}

void OrderDetailDialog::onJsonReceived(const QJsonObject &obj)
{
    const QString type = obj.value(Protocol::KEY_TYPE).toString();

    // 取消订单
    if (m_waitingCancelResp) {
        if (type == Protocol::TYPE_ERROR) {
            m_waitingCancelResp = false;
            refreshPayButton();
            refreshCancelButton();
            QMessageBox::critical(this, "取消失败", obj.value(Protocol::KEY_MESSAGE).toString());
            return;
        }

        if (type == Protocol::TYPE_ORDER_CANCEL_RESP) {
            m_waitingCancelResp = false;

            // 本地更新状态
            m_ord.status = Common::OrderStatus::Canceled;
            ui->lblStatus->setText(statusToText(m_ord.status));
            refreshPayButton();
            refreshCancelButton();

            QMessageBox::information(this, "成功", obj.value(Protocol::KEY_MESSAGE).toString());
            emit orderCanceled(m_ord.id);
            accept();
            return;
        }
    }

    if (!m_waitingPayResp) return;

    // 支付失败
    if (type == Protocol::TYPE_ERROR) {
        const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();
        if (!msg.contains("支付")) return;

        m_waitingPayResp = false;
        refreshPayButton();
        QMessageBox::critical(this, "支付失败", obj.value(Protocol::KEY_MESSAGE).toString());
        return;
    }

    // 支付成功
    if (type == Protocol::TYPE_ORDER_PAY_RESP) {
        m_waitingPayResp = false;

        // 本地更新：状态变 Paid，并刷新显示与按钮
        m_ord.status = Common::OrderStatus::Paid;
        ui->lblStatus->setText(statusToText(m_ord.status));
        refreshPayButton();

        QMessageBox::information(this, "成功", obj.value(Protocol::KEY_MESSAGE).toString());
        emit orderPaid(m_ord.id);
        return;
    }
}

void OrderDetailDialog::refreshCancelButton()
{
    bool enable = false;
    QString text = "不可取消";

    if (NetworkManager::instance()->m_username.isEmpty()) {
        enable = false;
        text = "未登录不可取消订单";
    } else if (m_ord.status == Common::OrderStatus::Canceled) {
        enable = false;
        text = "已退票不可取消订单";
    } else if (m_ord.status == Common::OrderStatus::Finished) {
        enable = false;
        text = "已完成不可取消订单";
    } else if (m_ord.status == Common::OrderStatus::Rescheduled) {
        enable = false;
        text = "已改签不可取消订单";
    } else {
        enable = !m_waitingCancelResp && !m_waitingPayResp;
        text = m_waitingCancelResp ? "取消中..." : "取消订单";
    }

    ui->btnCancel->setEnabled(enable);
    ui->btnCancel->setText(text);
}

void OrderDetailDialog::on_btnCancel_clicked()
{
    if (NetworkManager::instance()->m_username.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录，无法取消订单。");
        refreshCancelButton();
        return;
    }

    if (m_ord.status == Common::OrderStatus::Canceled) {
        QMessageBox::information(this, "提示", "该订单已退票。");
        refreshCancelButton();
        return;
    }

    if (m_ord.status == Common::OrderStatus::Finished) {
        QMessageBox::warning(this, "提示", "该订单已完成，无法取消。");
        refreshCancelButton();
        return;
    }

    if (m_ord.status == Common::OrderStatus::Rescheduled) {
        QMessageBox::warning(this, "提示", "该订单已改签，无法取消。");
        refreshCancelButton();
        return;
    }

    auto reply = QMessageBox::question(this, "确认取消",
                                       QString("确定要取消订单 %1 吗？").arg(m_ord.id));
    if (reply != QMessageBox::Yes) return;

    m_waitingCancelResp = true;
    refreshPayButton();
    refreshCancelButton();

    QJsonObject data;
    data.insert("username", NetworkManager::instance()->m_username);
    data.insert("orderId", static_cast<qint64>(m_ord.id));

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_CANCEL);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

