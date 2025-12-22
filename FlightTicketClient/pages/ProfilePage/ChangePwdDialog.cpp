#include "ChangePwdDialog.h"
#include "ui_ChangePwdDialog.h"
#include <QMessageBox>

static const char* kStyleNormal = "";
static const char* kStyleOk =
    "QLineEdit{border:1px solid #22c55e; border-radius:4px; padding:4px;}";
static const char* kStyleErr =
    "QLineEdit{border:1px solid #ef4444; border-radius:4px; padding:4px;}";

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
    const bool ok = validateOldPwd() && validateNewPwd() && validateNewPwd2();
    if (!ok) return;

    emit changePwdSubmitted(ui->leOldPwd->text(), ui->leNewPwd->text());
}


void ChangePwdDialog::on_btnCancel_clicked()
{
    reject();
}

static void setLineEditState(QLineEdit* le, bool ok, bool touched = true)
{
    if (!le) return;
    if (!touched) { le->setStyleSheet(kStyleNormal); return; }
    le->setStyleSheet(ok ? kStyleOk : kStyleErr);
}

static bool isStrongPassword(const QString& p)
{
    if (p.length() < 8 || p.length() > 20) return false;

    bool hasUpper = false, hasLower = false, hasDigit = false;
    for (QChar ch : p) {
        if (ch.isUpper()) hasUpper = true;
        else if (ch.isLower()) hasLower = true;
        else if (ch.isDigit()) hasDigit = true;
    }
    return hasUpper && hasLower && hasDigit;
}

bool ChangePwdDialog::validateOldPwd()
{
    const QString oldPwd = ui->leOldPwd->text();

    if (oldPwd.isEmpty()) {
        ui->lblErrOldPwd->setText("原密码不能为空");
        setLineEditState(ui->leOldPwd, false, false);
        return false;
    }

    ui->lblErrOldPwd->setText("");
    setLineEditState(ui->leOldPwd, true);
    return true;
}

bool ChangePwdDialog::validateNewPwd()
{
    const QString newPwd = ui->leNewPwd->text();

    if (newPwd.isEmpty()) {
        ui->lblErrNewPwd->setText("新密码不能为空");
        setLineEditState(ui->leNewPwd, false, false);
        return false;
    }
    if (!isStrongPassword(newPwd)) {
        ui->lblErrNewPwd->setText("新密码需8-20位且包含大小写字母和数字");
        setLineEditState(ui->leNewPwd, false);
        return false;
    }

    if (!ui->leOldPwd->text().isEmpty() && ui->leOldPwd->text() == newPwd) {
        ui->lblErrNewPwd->setText("新密码不能与原密码相同");
        setLineEditState(ui->leNewPwd, false);
        return false;
    }

    ui->lblErrNewPwd->setText("");
    setLineEditState(ui->leNewPwd, true);
    return true;
}

bool ChangePwdDialog::validateNewPwd2()
{
    const QString newPwd = ui->leNewPwd->text();
    const QString newPwd2 = ui->leNewPwd2->text();

    if (newPwd2.isEmpty()) {
        ui->lblErrNewPwd2->setText("请再次输入新密码");
        setLineEditState(ui->leNewPwd2, false, false);
        return false;
    }
    if (newPwd != newPwd2) {
        ui->lblErrNewPwd2->setText("两次新密码不一致");
        setLineEditState(ui->leNewPwd2, false);
        return false;
    }

    ui->lblErrNewPwd2->setText("");
    setLineEditState(ui->leNewPwd2, true);
    return true;
}

void ChangePwdDialog::updateSubmitEnabled()
{
    const bool ok = validateOldPwd() && validateNewPwd() && validateNewPwd2();
    ui->btnOk->setEnabled(ok);
}

void ChangePwdDialog::on_leOldPwd_textEdited(const QString &)
{
    validateOldPwd();
    validateNewPwd();
    updateSubmitEnabled();
}


void ChangePwdDialog::on_leNewPwd_textEdited(const QString &)
{
    validateNewPwd();
    validateNewPwd2();
    updateSubmitEnabled();
}


void ChangePwdDialog::on_leNewPwd2_textEdited(const QString &)
{
    validateNewPwd2();
    updateSubmitEnabled();
}

