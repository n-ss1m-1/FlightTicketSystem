#ifndef ORDERDETAILDIALOG_H
#define ORDERDETAILDIALOG_H

#include <QDialog>
#include "Common/Models.h"

namespace Ui {
class OrderDetailDialog;
}

class OrderDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OrderDetailDialog(QWidget *parent = nullptr);
    ~OrderDetailDialog();

    void setData(const Common::OrderInfo &ord,
                 const Common::FlightInfo &flt,
                 const QString &sourceText);

signals:
    void orderPaid(qint64 orderId); // 支付后刷新OrderPage
    void orderCanceled(qint64 orderId);

private slots:
    void on_btnPay_clicked();
    void on_btnExit_clicked();
    void onJsonReceived(const QJsonObject &obj);

    void on_btnCancel_clicked();
    void on_btnReschedule_clicked();

    void on_cbShowPassengerPrivacy_toggled(bool checked);

private:
    Ui::OrderDetailDialog *ui;

    static QString statusToText(Common::OrderStatus s);
    static QString dtToText(const QDateTime &dt);

    void refreshPayButton();

    Common::OrderInfo  m_ord;     // 保存当前订单用于支付/更新状态
    Common::FlightInfo m_flt;     //
    QString m_sourceText;

    bool m_waitingPayResp = false;
    QMetaObject::Connection m_conn;

    bool m_waitingCancelResp = false;
    void refreshCancelButton();

    void refreshPriceLabel();

    void refreshRescheduleButton();

    static QString maskMiddle(const QString& s, int left, int right, QChar ch = '*');
    void applyPassengerPrivacyMask();
};

#endif // ORDERDETAILDIALOG_H
