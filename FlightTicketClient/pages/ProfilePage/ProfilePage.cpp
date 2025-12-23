#include "ProfilePage.h"
#include "ui_ProfilePage.h"
#include "LoginDialog.h"
#include <QMessageBox>
#include "NetworkManager.h"
#include "RegisterDialog.h"
#include "ChangePwdDialog.h"

ProfilePage::ProfilePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProfilePage)
{
    ui->setupUi(this);
    updateLoginUI();
}

ProfilePage::~ProfilePage()
{
    delete ui;
}

void ProfilePage::updateLoginUI()
{
    if (m_loggedIn) {
        ui->btnLogin->setText("退出登录");
        ui->lblStatus->setText("已登录：" + m_username);
        ui->btnRegister->setText("修改密码");
    } else {
        ui->btnLogin->setText("登录");
        ui->lblStatus->setText("未登录");
        ui->btnRegister->setText("注册");
    }
}

void ProfilePage::updateUserInfoUI()
{
    if (!m_loggedIn) {
        ui->lblPhoneValue->setText("-");
        ui->lblRealNameValue->setText("-");
        ui->lblIdCardValue->setText("-");
        ui->btnChangePhone->setEnabled(false);
        return;
    }

    ui->btnChangePhone->setEnabled(true);

    applyPrivacyMask();
}

QString ProfilePage::maskMiddle(const QString& s, int left, int right, QChar ch)
{
    if (s.isEmpty()) return "";
    if (left + right >= s.size()) return QString(s.size(), ch);

    QString out = s.left(left);
    out += QString(s.size() - left - right, ch);
    out += s.right(right);
    return out;
}

void ProfilePage::applyPrivacyMask()
{
    const QString phone    = m_userJson.value("phone").toString();
    const QString realName = m_userJson.value("realName").toString();
    const QString idCard   = m_userJson.value("idCard").toString();

    // 手机号：显示 3-4-4，其余*
    ui->lblPhoneValue->setText(
        ui->cbShowPhone->isChecked() ? phone : maskMiddle(phone, 3, 4)
        );

    // 真实姓名：显示最后一个字，其余*
    ui->lblRealNameValue->setText(
        ui->cbShowRealName->isChecked() ? realName : (realName.isEmpty() ? "" : maskMiddle(realName, 0, 1))
        );

    // 身份证：显示前4后4
    ui->lblIdCardValue->setText(
        ui->cbShowIdCard->isChecked() ? idCard : maskMiddle(idCard, 4, 4)
        );
}

void ProfilePage::on_btnLogin_clicked()
{
    // 已登录：退出登录
    if (m_loggedIn) {
        QMessageBox box(this);
        box.setWindowTitle("确认退出");
        box.setText("确定要退出登录吗？");
        if (!m_username.isEmpty())
            box.setInformativeText("当前用户：" + m_username);

        QPushButton *btnLogout = box.addButton("确认", QMessageBox::AcceptRole);
        QPushButton *btnCancel = box.addButton("取消", QMessageBox::RejectRole);

        box.setDefaultButton(btnCancel);
        box.exec();

        if (box.clickedButton() == btnLogout) {
            m_loggedIn = false;
            NetworkManager::instance()->m_username = "";
            m_username.clear();
            m_userJson = QJsonObject();
            updateUserInfoUI();
            updateLoginUI();
        }
        return;
    }

    auto *dlg = new LoginDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    // 发登录请求
    connect(dlg, &LoginDialog::loginSubmitted, this,
            [this](const QString &u, const QString &p) {
                NetworkManager::instance()->login(u, p);
            });

    connect(NetworkManager::instance(), &NetworkManager::jsonReceived, dlg,
            [this, dlg](const QJsonObject &resp) {
                const QString type = resp.value(Protocol::KEY_TYPE).toString();

                // 服务端登录成功：type=login_response, success=true, data=UserInfo
                if (type == Protocol::TYPE_LOGIN_RESP) {
                    const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                    const QJsonObject data = resp.value(Protocol::KEY_DATA).toObject();

                    if (success) {
                        m_loggedIn = true;

                        NetworkManager::instance()->m_username = data.value("username").toString();

                        m_username = data.value("username").toString();
                        if (m_username.isEmpty()) m_username = data.value("realName").toString();
                        m_userJson = data;
                        updateUserInfoUI();
                        updateLoginUI();
                        dlg->accept();
                    } else {
                        QMessageBox::warning(dlg, "登录失败", msg.isEmpty() ? "登录失败" : msg);
                    }
                    return;
                }

                // 服务端登录失败：type=error, message="账号或密码错误"
                if (type == Protocol::TYPE_ERROR) {
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                    QMessageBox::warning(dlg, "登录失败", msg.isEmpty() ? "账号或密码错误" : msg);
                    return;
                }
            });


    dlg->exec();
}

void ProfilePage::on_btnRegister_clicked()
{
    // 未登录：注册
    if (!m_loggedIn) {
        auto *dlg = new RegisterDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        connect(dlg, &RegisterDialog::registerSubmitted, this,
                [dlg](const QString &u, const QString &p,
                      const QString &phone, const QString &realName, const QString &idCard) {
                    NetworkManager::instance()->registerUser(u, p, phone, realName, idCard);
                });

        connect(NetworkManager::instance(), &NetworkManager::jsonReceived, dlg,
                [dlg](const QJsonObject &resp) {
                    const QString type = resp.value(Protocol::KEY_TYPE).toString();

                    if (type == Protocol::TYPE_REGISTER_RESP) {
                        const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                        const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();

                        if (success) {
                            QMessageBox::information(dlg, "注册成功", msg.isEmpty() ? "注册成功" : msg);
                            dlg->accept();
                        } else {
                            QMessageBox::warning(dlg, "注册失败", msg.isEmpty() ? "注册失败" : msg);
                        }
                        return;
                    }

                    if (type == Protocol::TYPE_ERROR) {
                        const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                        QMessageBox::warning(dlg, "注册失败", msg.isEmpty() ? "注册失败" : msg);
                        return;
                    }
                });

        dlg->exec();
        return;
    }

    // 已登录：修改密码
    auto *dlg = new ChangePwdDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    connect(dlg, &ChangePwdDialog::changePwdSubmitted, this,
            [this](const QString &oldPwd, const QString &newPwd) {
                NetworkManager::instance()->changePassword(m_username, oldPwd, newPwd);
            });

    connect(NetworkManager::instance(), &NetworkManager::jsonReceived, dlg,
            [dlg](const QJsonObject &resp) {
                const QString type = resp.value(Protocol::KEY_TYPE).toString();

                if (type == Protocol::TYPE_CHANGE_PWD_RESP) {
                    const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();

                    if (success) {
                        QMessageBox::information(dlg, "修改成功", msg.isEmpty() ? "密码修改成功" : msg);
                        dlg->accept();
                    } else {
                        QMessageBox::warning(dlg, "修改失败", msg.isEmpty() ? "密码修改失败" : msg);
                    }
                    return;
                }

                if (type == Protocol::TYPE_ERROR) {
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                    QMessageBox::warning(dlg, "修改失败", msg.isEmpty() ? "修改失败" : msg);
                    return;
                }
            });

    dlg->exec();
}





void ProfilePage::on_cbShowPhone_toggled(bool)
{
    if (m_loggedIn) applyPrivacyMask();
}


void ProfilePage::on_cbShowRealName_toggled(bool)
{
    if (m_loggedIn) applyPrivacyMask();
}

void ProfilePage::on_cbShowIdCard_toggled(bool)
{
    if (m_loggedIn) applyPrivacyMask();
}

