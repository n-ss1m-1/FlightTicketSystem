#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowTitle("登录");
    setModal(true);

    connect(ui->lePassword, &QLineEdit::returnPressed, this, &LoginDialog::on_btnLogin_clicked);
    connect(ui->leUsername, &QLineEdit::returnPressed, this, &LoginDialog::on_btnLogin_clicked);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

QString LoginDialog::username() const { return ui->leUsername->text().trimmed(); }
QString LoginDialog::password() const { return ui->lePassword->text(); }

void LoginDialog::on_btnCancel_clicked()
{
    reject();
}

void LoginDialog::on_btnLogin_clicked()
{
    const QString u = username();
    const QString p = password();

    if (u.isEmpty() || p.isEmpty()) {
        QMessageBox::warning(this, "提示", "用户名或密码不能为空");
        return;
    }

    emit loginSubmitted(u, p);
}

void LoginDialog::on_btnGoRegister_clicked()
{
    emit requestRegister();
    reject();
}

