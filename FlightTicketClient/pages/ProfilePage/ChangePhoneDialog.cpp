#include "ChangePhoneDialog.h"
#include "ui_ChangePhoneDialog.h"
#include <QRegularExpression>
#include <QLineEdit>

static const char* kStyleNormal = "";
static const char* kStyleOk =
    "QLineEdit{border:1px solid #22c55e; border-radius:4px; padding:4px;}";
static const char* kStyleErr =
    "QLineEdit{border:1px solid #ef4444; border-radius:4px; padding:4px;}";

ChangePhoneDialog::ChangePhoneDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChangePhoneDialog)
{
    ui->setupUi(this);

    setWindowTitle("修改手机号");
    setModal(true);
}

static void setLineEditState(QLineEdit* le, bool ok, bool touched = true)
{
    if (!le) return;
    if (!touched) { le->setStyleSheet(kStyleNormal); return; }
    le->setStyleSheet(ok ? kStyleOk : kStyleErr);
}

ChangePhoneDialog::~ChangePhoneDialog()
{
    delete ui;
}

void ChangePhoneDialog::setCurrentPhone(const QString &phone)
{
    ui->leNewPhone->setText(phone);
    validatePhone();
    updateSubmitEnabled();
}

bool ChangePhoneDialog::validatePhone()
{
    const QString phone = ui->leNewPhone->text().trimmed();

    if (phone.isEmpty()) {
        ui->lblErrPhone->setText("手机号不能为空");
        setLineEditState(ui->leNewPhone, false, false);
        return false;
    }

    static const QRegularExpression re(R"(^\d{11}$)");
    if (!re.match(phone).hasMatch()) {
        ui->lblErrPhone->setText("无效的手机号");
        setLineEditState(ui->leNewPhone, false);
        return false;
    }

    ui->lblErrPhone->setText("");
    setLineEditState(ui->leNewPhone, true);
    return true;
}

void ChangePhoneDialog::updateSubmitEnabled()
{
    ui->btnOk->setEnabled(validatePhone());
}

void ChangePhoneDialog::on_leNewPhone_textEdited(const QString &)
{
    updateSubmitEnabled();
}

void ChangePhoneDialog::on_btnOk_clicked()
{
    if (!validatePhone()) return;
    emit phoneSubmitted(ui->leNewPhone->text().trimmed());
}

void ChangePhoneDialog::on_btnCancel_clicked()
{
    reject();
}
