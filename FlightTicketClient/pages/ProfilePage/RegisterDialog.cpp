#include "RegisterDialog.h"
#include "ui_RegisterDialog.h"
#include <QMessageBox>
#include <QRegularExpression>

static const char* kStyleNormal = "";
static const char* kStyleOk = // lineEdit 正确输入样式
    "QLineEdit{border:1px solid #22c55e; border-radius:4px; padding:4px;}";
static const char* kStyleErr = // lineEdit 错误输入样式
    "QLineEdit{border:1px solid #ef4444; border-radius:4px; padding:4px;}";


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
    const bool ok =
        validateUsername() &&
        validatePassword() &&
        validatePassword2() &&
        validatePhone() &&
        validateIdCard() &&
        validateRealName();

    if (!ok) return;

    const QString u = ui->leUsername->text().trimmed();
    const QString p = ui->lePassword->text();
    const QString phone = ui->lePhone->text().trimmed();
    const QString realName = ui->leRealName->text().trimmed();
    const QString idCard = ui->leIdCard->text().trimmed();

    emit registerSubmitted(u, p, phone, realName, idCard);
}

static void setLineEditState(QLineEdit* le, bool ok, bool touched = true)
{
    if (!le) return;

    if (!touched) {
        le->setStyleSheet(kStyleNormal);
        return;
    }
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

bool RegisterDialog::validatePassword()
{
    const QString p = ui->lePassword->text();

    if (p.isEmpty()) {
        ui->lblErrPassword->setText("密码不能为空");
        setLineEditState(ui->lePassword, false, false);
        return false;
    }
    if (!isStrongPassword(p)) {
        ui->lblErrPassword->setText("密码需8-20位且包含大小写字母和数字");
        setLineEditState(ui->lePassword, false);
        return false;
    }

    ui->lblErrPassword->setText("");
    setLineEditState(ui->lePassword, true);
    return true;
}

bool RegisterDialog::validatePassword2()
{
    const QString p = ui->lePassword->text();
    const QString p2 = ui->lePassword2->text();

    if (p2.isEmpty()) {
        ui->lblErrPassword2->setText("请再次输入密码");
        setLineEditState(ui->lePassword2, false, false);
        return false;
    }
    if (p != p2) {
        ui->lblErrPassword2->setText("两次密码不一致");
        setLineEditState(ui->lePassword2, false);
        return false;
    }

    ui->lblErrPassword2->setText("");
    setLineEditState(ui->lePassword2, true);
    return true;
}

bool RegisterDialog::validatePhone()
{
    const QString phone = ui->lePhone->text().trimmed();

    if (phone.isEmpty()) {
        ui->lblErrPhone->setText("手机号不能为空");
        setLineEditState(ui->lePhone, false, false);
        return false;
    }

    static const QRegularExpression re(R"(^\d{11}$)");
    if (!re.match(phone).hasMatch()) {
        ui->lblErrPhone->setText("无效的手机号");
        setLineEditState(ui->lePhone, false);
        return false;
    }

    ui->lblErrPhone->setText("");
    setLineEditState(ui->lePhone, true);
    return true;
}

bool RegisterDialog::validateIdCard()
{
    const QString idCard = ui->leIdCard->text().trimmed();

    if (idCard.isEmpty()) {
        ui->lblErrIdCard->setText("身份证号不能为空");
        setLineEditState(ui->leIdCard, false, false);
        return false;
    }

    static const QRegularExpression re(R"(^\d{17}(\d|X|x)$)");
    if (!re.match(idCard).hasMatch()) {
        ui->lblErrIdCard->setText("无效的身份证号");
        setLineEditState(ui->leIdCard, false);
        return false;
    }

    ui->lblErrIdCard->setText("");
    setLineEditState(ui->leIdCard, true);
    return true;
}

bool RegisterDialog::validateUsername()
{
    const QString u = ui->leUsername->text().trimmed();
    if (u.isEmpty()) {
        ui->lblErrUserName->setText("用户名不能为空");
        setLineEditState(ui->leUsername, false, false);
        return false;
    }
    if (u.length() < 3 || u.length() > 20) {
        ui->lblErrUserName->setText("用户名长度需3-20位");
        setLineEditState(ui->leUsername, false);
        return false;
    }

    ui->lblErrUserName->setText("");
    setLineEditState(ui->leUsername, true);
    return true;
}

bool RegisterDialog::validateRealName()
{
    const QString name = ui->leRealName->text().trimmed();

    if (name.isEmpty()) {
        ui->lblErrRealName->setText("姓名不能为空");
        setLineEditState(ui->leRealName, false, false);
        return false;
    }

    static const QRegularExpression hasLetterOrDigit(R"([A-Za-z0-9])");
    if (hasLetterOrDigit.match(name).hasMatch()) {
        ui->lblErrRealName->setText("姓名不能包含字母或数字");
        setLineEditState(ui->leRealName, false);
        return false;
    }

    ui->lblErrRealName->setText("");
    setLineEditState(ui->leRealName, true);
    return true;
}

void RegisterDialog::updateSubmitEnabled()
{
    const bool ok =
        validateUsername() &&
        validatePassword() &&
        validatePassword2() &&
        validatePhone() &&
        validateIdCard() &&
        validateRealName();

    ui->btnOk->setEnabled(ok);
}


void RegisterDialog::on_leUsername_textEdited(const QString &)
{
    validateUsername();
    updateSubmitEnabled();
}

void RegisterDialog::on_lePassword_textEdited(const QString &)
{
    validatePassword();
    validatePassword2();
    updateSubmitEnabled();
}

void RegisterDialog::on_lePassword2_textEdited(const QString &)
{
    validatePassword2();
    updateSubmitEnabled();
}

void RegisterDialog::on_lePhone_textEdited(const QString &)
{
    validatePhone();
    updateSubmitEnabled();
}

void RegisterDialog::on_leIdCard_textEdited(const QString &)
{
    validateIdCard();
    updateSubmitEnabled();
}


void RegisterDialog::on_leRealName_textEdited(const QString &arg1)
{
    validateRealName();
    updateSubmitEnabled();
}

