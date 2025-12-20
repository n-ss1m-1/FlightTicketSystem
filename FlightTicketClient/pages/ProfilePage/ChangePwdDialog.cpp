#include "ChangePwdDialog.h"
#include "ui_ChangePwdDialog.h"
#include <QMessageBox>

ChangePwdDialog::ChangePwdDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChangePwdDialog)
{
    ui->setupUi(this);
    setWindowTitle("修改密码");
    setModal(true);
}

ChangePwdDialog::~ChangePwdDialog()
{
    delete ui;
}

void ChangePwdDialog::on_btnOk_clicked()
{
    const QString oldPwd = ui->leOldPwd->text();
    const QString newPwd = ui->leNewPwd->text();
    const QString newPwd2 = ui->leNewPwd2->text();

    if (oldPwd.isEmpty() || newPwd.isEmpty() || newPwd2.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写完整");
        return;
    }
    if (newPwd != newPwd2) {
        QMessageBox::warning(this, "提示", "两次新密码不一致");
        return;
    }
    emit changePwdSubmitted(oldPwd, newPwd);
}


void ChangePwdDialog::on_btnCancel_clicked()
{
    reject();
}

