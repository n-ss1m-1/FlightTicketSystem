#ifndef ADDFLIGHTDIALOG_H
#define ADDFLIGHTDIALOG_H

#include <QDialog>

namespace Ui {
class AddFlightDialog;
}

class AddFlightDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFlightDialog(QWidget *parent = nullptr);
    ~AddFlightDialog();

private slots:
    void on_btnConfirm_clicked(); // 确认按钮
    void on_btnCancel_clicked();  // 取消按钮

private:
    Ui::AddFlightDialog *ui;
};

#endif // ADDFLIGHTDIALOG_H
