#include "RegisterDialog.h"
#include "ui_RegisterDialog.h"
#include <QMessageBox>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    setWindowTitle("注册");
    setModal(true);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_btnCancel_clicked()
{
    reject();
}

void RegisterDialog::on_btnOk_clicked()
{
    const QString u = ui->leUsername->text().trimmed();
    const QString p = ui->lePassword->text();
    const QString p2 = ui->lePassword2->text();
    const QString phone = ui->lePhone->text().trimmed();
    const QString realName = ui->leRealName->text().trimmed();
    const QString idCard = ui->leIdCard->text().trimmed();

    if (u.isEmpty() || p.isEmpty() || phone.isEmpty() || realName.isEmpty() || idCard.isEmpty()) {
        QMessageBox::warning(this, "提示", "请把信息填写完整");
        return;
    }

    if (p != p2) {
        QMessageBox::warning(this, "提示", "两次输入的密码不一致");
        ui->lePassword2->setFocus();
        ui->lePassword2->selectAll();
        return;
    }

    emit registerSubmitted(u, p, phone, realName, idCard);
}
