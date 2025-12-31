#include "OrderDetailDialog.h"
#include "ui_OrderDetailDialog.h"

#include "NetworkManager.h"
#include "Common/Protocol.h"
#include <QJsonObject>
#include <QMessageBox>
#include "RescheduleDialog.h"

static QString centsToYuanText(qint32 cents)
{
    return QString::number(cents / 100.0, 'f', 2);
}

QString OrderDetailDialog::maskMiddle(const QString& s, int left, int right, QChar ch)
{
    if (s.isEmpty()) return "";
    if (left + right >= s.size()) return QString(s.size(), ch);

    QString out = s.left(left);
    out += QString(s.size() - left - right, ch);
    out += s.right(right);
    return out;
}

void OrderDetailDialog::applyPassengerPrivacyMask()
{
    const QString name = m_ord.passengerName;
    const QString id   = m_ord.passengerIdCard;

    const bool show = ui->cbShowPassengerPrivacy->isChecked();

    // 姓名：显示最后一个字，其余*
    const QString showName = show
                                 ? name
                                 : (name.isEmpty() ? "" : maskMiddle(name, 0, 1));

    // 身份证：显示前4后4
    const QString showId = show
                               ? id
                               : maskMiddle(id, 4, 4);

    ui->lblPassengerName->setText(name.isEmpty() ? "--" : showName);
    ui->lblPassengerIdCard->setText(id.isEmpty() ? "--" : showId);
}

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
    case Common::OrderStatus::Booked:   return "待支付";
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

void OrderDetailDialog::refreshPriceLabel(){
    const qint32 paid = m_ord.priceCents - m_ord.pendingPayment;
    ui->lblPrice->setText(
        QString("总价：￥%1\n已付：￥%2\n待付：￥%3")
            .arg(centsToYuanText(m_ord.priceCents))
            .arg(centsToYuanText(paid))
            .arg(centsToYuanText(m_ord.pendingPayment))
        );
}

void OrderDetailDialog::refreshPayButton()
{
    // 未登录/已取消/已完成/已改签：不可支付
    if (!NetworkManager::instance()->isLoggedIn() ||
        m_ord.status == Common::OrderStatus::Canceled ||
        m_ord.status == Common::OrderStatus::Finished ||
        m_ord.status == Common::OrderStatus::Rescheduled) {
        ui->btnPay->setEnabled(false);
        ui->btnPay->setText("不可支付");
        return;
    }

    // 没有待支付金额：视为已支付完成
    if (m_ord.pendingPayment <= 0) {
        ui->btnPay->setEnabled(false);
        ui->btnPay->setText("已支付");
        return;
    }

    // 有待支付金额：允许支付
    ui->btnPay->setEnabled(!m_waitingPayResp && !m_waitingCancelResp);
    ui->btnPay->setText(m_waitingPayResp
        ? "支付中..."
        : QString("支付 ¥%1").arg(centsToYuanText(m_ord.pendingPayment)));
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
    // ui->lblPrice->setText(QString("￥%1").arg(QString::number(ord.priceCents / 100.0, 'f', 2)));
    refreshPriceLabel();

    // 乘机人信息
    applyPassengerPrivacyMask();
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
    refreshRescheduleButton();
}

void OrderDetailDialog::on_btnPay_clicked()
{
    if (NetworkManager::instance()->m_username.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录，无法支付。");
        refreshPayButton();
        return;
    }

    if (m_ord.status == Common::OrderStatus::Canceled ||
        m_ord.status == Common::OrderStatus::Finished ||
        m_ord.status == Common::OrderStatus::Rescheduled) {
        QMessageBox::warning(this, "提示", "该订单当前状态不可支付。");
        refreshPayButton();
        return;
    }

    if (m_ord.pendingPayment <= 0) {
        QMessageBox::information(this, "提示", "该订单没有待支付金额。");
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

    qDebug() << "[OrderDetailDialog] recv type =" << type
             << "waiting=" << m_waitingPayResp;

    // 取消订单
    if (m_waitingCancelResp) {
        if (type == Protocol::TYPE_ERROR) {
            m_waitingCancelResp = false;
            refreshPayButton();
            refreshCancelButton();
            refreshRescheduleButton();
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
            refreshRescheduleButton();

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
        m_ord.pendingPayment = 0;
        ui->lblStatus->setText(statusToText(m_ord.status));

        refreshPriceLabel();
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

    if (!NetworkManager::instance()->isLoggedIn()) {
        enable = false;
        text = "未登录";
    } else if (m_ord.status == Common::OrderStatus::Canceled) {
        enable = false;
        text = "不可取消已退票订单";
    } else if (m_ord.status == Common::OrderStatus::Finished) {
        enable = false;
        text = "不可取消已完成订单";
    } else if (m_ord.status == Common::OrderStatus::Rescheduled) {
        enable = false;
        text = "不可取消已改签订单";
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
    refreshRescheduleButton();

    QJsonObject data;
    data.insert("username", NetworkManager::instance()->m_username);
    data.insert("orderId", static_cast<qint64>(m_ord.id));

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_CANCEL);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

void OrderDetailDialog::on_btnReschedule_clicked()
{
    if (NetworkManager::instance()->m_username.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录，无法改签。");
        return;
    }

    if (m_sourceText != "本用户") {
        QMessageBox::warning(this, "提示", "仅本用户的订单允许改签。");
        return;
    }

    if (m_ord.status == Common::OrderStatus::Canceled ||
        m_ord.status == Common::OrderStatus::Finished ||
        m_ord.status == Common::OrderStatus::Rescheduled) {
        QMessageBox::warning(this, "提示", "该订单状态不可改签。");
        return;
    }

    RescheduleDialog dlg(this);
    dlg.setData(m_ord, m_flt);

    connect(&dlg, &RescheduleDialog::rescheduled, this,
            [&](const Common::OrderInfo &newOrder,
                const Common::FlightInfo &newFlight,
                qint32)
            {
                // 改签成功后：把详情页切到新订单
                m_ord = newOrder;
                m_flt = newFlight;

                ui->lblOrderId->setText(QString::number(m_ord.id));
                ui->lblStatus->setText(statusToText(m_ord.status));

                refreshPriceLabel();

                // 航班展示更新
                ui->lblFlightNo->setText(m_flt.flightNo.isEmpty() ? "--" : m_flt.flightNo);
                ui->lblRoute->setText(QString("%1 → %2")
                                          .arg(m_flt.fromCity.isEmpty() ? "--" : m_flt.fromCity)
                                          .arg(m_flt.toCity.isEmpty() ? "--" : m_flt.toCity));
                ui->lblDepartTime->setText(dtToText(m_flt.departTime));
                ui->lblArriveTime->setText(dtToText(m_flt.arriveTime));
                ui->lblSeatNum->setText(m_ord.seatNum.isEmpty() ? "--" : m_ord.seatNum);

                // 刷新按钮：如果补差价 pendingPayment>0，支付按钮应可用
                m_waitingPayResp = false;
                m_waitingCancelResp = false;
                refreshPayButton();
                refreshCancelButton();
                refreshRescheduleButton();
            });

    dlg.exec();
}

void OrderDetailDialog::refreshRescheduleButton()
{
    bool enable = false;
    QString text = "不可改签";

    if (!NetworkManager::instance()->isLoggedIn()) {
        enable = false;
        text = "未登录";
    } else if (m_sourceText != "本用户") {
        enable = false;
        text = "仅本用户订单可改签";
    } else if (m_ord.status == Common::OrderStatus::Canceled) {
        enable = false;
        text = "已退票不可改签";
    } else if (m_ord.status == Common::OrderStatus::Finished) {
        enable = false;
        text = "已完成不可改签";
    } else if (m_ord.status == Common::OrderStatus::Rescheduled) {
        enable = false;
        text = "已改签";
    } else {
        enable = !m_waitingPayResp && !m_waitingCancelResp;
        text = enable ? "改签" : "处理中...";
    }

    ui->btnReschedule->setEnabled(enable);
    ui->btnReschedule->setText(text);
}

void OrderDetailDialog::on_cbShowPassengerPrivacy_toggled(bool)
{
    applyPassengerPrivacyMask();
}

