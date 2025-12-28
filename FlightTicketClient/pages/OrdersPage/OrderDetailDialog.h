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

    void setOrder(const Common::OrderInfo &ord, const QString &sourceText = QString());


private:
    Ui::OrderDetailDialog *ui;
};

#endif // ORDERDETAILDIALOG_H
