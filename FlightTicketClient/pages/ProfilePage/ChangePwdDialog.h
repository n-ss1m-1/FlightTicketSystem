#ifndef CHANGEPWDDIALOG_H
#define CHANGEPWDDIALOG_H

#include <QDialog>

namespace Ui {
class ChangePwdDialog;
}

class ChangePwdDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePwdDialog(QWidget *parent = nullptr);
    ~ChangePwdDialog();

signals:
    void changePwdSubmitted(const QString &oldPwd, const QString &newPwd);

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

private:
    Ui::ChangePwdDialog *ui;
};

#endif // CHANGEPWDDIALOG_H
