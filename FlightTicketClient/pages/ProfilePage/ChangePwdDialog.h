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

    void on_leOldPwd_textEdited(const QString &arg1);

    void on_leNewPwd_textEdited(const QString &arg1);

    void on_leNewPwd2_textEdited(const QString &arg1);

private:
    Ui::ChangePwdDialog *ui;

    bool validateOldPwd();
    bool validateNewPwd();
    bool validateNewPwd2();
    void updateSubmitEnabled();

};

#endif // CHANGEPWDDIALOG_H
