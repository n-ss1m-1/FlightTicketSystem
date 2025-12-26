#include "AddUserDialog.h"
#include "ui_AddUserDialog.h"
#include "DBManager.h"
#include <QMessageBox>

AddUserDialog::AddUserDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddUserDialog)
{
    ui->setupUi(this);
    setWindowTitle("后台录入新用户");
}

AddUserDialog::~AddUserDialog()
{
    delete ui;
}

void AddUserDialog::on_btnCancel_clicked()
{
    reject();
}

void AddUserDialog::on_btnConfirm_clicked()
{
    // 1. 获取输入
    QString user = ui->editUsername->text().trimmed();
    QString pwd = ui->editPassword->text().trimmed();
    QString name = ui->editRealName->text().trimmed();
    QString idCard = ui->editIdCard->text().trimmed();
    QString phone = ui->editPhone->text().trimmed();

    // 2. 简单校验
    if (user.isEmpty() || pwd.isEmpty() || name.isEmpty() || idCard.isEmpty() || phone.isEmpty()) {
        QMessageBox::warning(this, "提示", "所有字段都必须填写！");
        return;
    }

    // 3. 插入数据库
    QString sql = "INSERT INTO user (username, password, real_name, id_card, phone) "
                  "VALUES (?, ?, ?, ?, ?)";
    QList<QVariant> params;
    params << user << pwd << name << idCard << phone;

    QString err;
    int ret = DBManager::instance().update(sql, params, &err);

    if (ret > 0) {
        QMessageBox::information(this, "成功", "用户添加成功！");
        accept();
    } else {
        // 如果用户名重复，数据库会报错，这里会弹窗显示 Duplicate entry...
        QMessageBox::critical(this, "失败", "添加失败: " + err);
    }
}
