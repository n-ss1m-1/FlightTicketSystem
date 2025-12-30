#include "AddPassengerDialog.h"
#include "ui_AddPassengerDialog.h"
#include <QRegularExpression>

static const char* kStyleNormal = "";
static const char* kStyleOk =
    "QLineEdit{border:1px solid #22c55e; border-radius:4px; padding:4px;}";
static const char* kStyleErr =
    "QLineEdit{border:1px solid #ef4444; border-radius:4px; padding:4px;}";

static void setLineEditState(QLineEdit* le, bool ok, bool touched = true)
{
    if (!le) return;

    if (!touched) {
        le->setStyleSheet(kStyleNormal);
        return;
    }
    le->setStyleSheet(ok ? kStyleOk : kStyleErr);
}

AddPassengerDialog::AddPassengerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddPassengerDialog)
{
    ui->setupUi(this);
    setWindowTitle("添加常用乘机人");
    setModal(true);

    ui->lblErrName->setText("");
    ui->lblErrIdCard->setText("");

    updateSubmitEnabled();
}

AddPassengerDialog::~AddPassengerDialog()
{
    delete ui;
}

void AddPassengerDialog::on_btnCancel_clicked()
{
    reject();
}

void AddPassengerDialog::on_btnOk_clicked()
{
    const bool ok = validateName() && validateIdCard();
    if (!ok) return;

    const QString name = ui->leName->text().trimmed();
    const QString idCard = ui->leIdCard->text().trimmed().toUpper();

    emit passengerSubmitted(name, idCard);
    accept();
}

bool AddPassengerDialog::validateName()
{
    const QString name = ui->leName->text().trimmed();

    if (name.isEmpty()) {
        ui->lblErrName->setText("姓名不能为空");
        setLineEditState(ui->leName, false, false);
        return false;
    }

    static const QRegularExpression hasLetterOrDigit(R"([A-Za-z0-9])");
    if (hasLetterOrDigit.match(name).hasMatch()) {
        ui->lblErrName->setText("姓名不能包含字母或数字");
        setLineEditState(ui->leName, false);
        return false;
    }

    ui->lblErrName->setText("");
    setLineEditState(ui->leName, true);
    return true;
}

bool AddPassengerDialog::validateIdCard()
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

void AddPassengerDialog::updateSubmitEnabled()
{
    const bool ok = validateName() && validateIdCard();
    ui->btnOk->setEnabled(ok);
}

void AddPassengerDialog::on_leName_textEdited(const QString &)
{
    validateName();
    updateSubmitEnabled();
}

void AddPassengerDialog::on_leIdCard_textEdited(const QString &)
{
    validateIdCard();
    updateSubmitEnabled();
}
