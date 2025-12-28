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


private:
    Ui::OrderDetailDialog *ui;

    static QString statusToText(Common::OrderStatus s);
    static QString dtToText(const QDateTime &dt);
};

#endif // ORDERDETAILDIALOG_H
