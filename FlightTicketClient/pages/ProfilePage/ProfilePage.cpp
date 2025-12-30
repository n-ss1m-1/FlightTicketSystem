#include "ProfilePage.h"
#include "ui_ProfilePage.h"
#include "LoginDialog.h"
#include <QMessageBox>
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "RegisterDialog.h"
#include "ChangePwdDialog.h"
#include "ChangePhoneDialog.h"
#include "SettingsManager.h"
#include <QLineEdit>
#include <QInputDialog>
#include <QSharedPointer>

ProfilePage::ProfilePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProfilePage)
{
    ui->setupUi(this);

    updateLoginUI();
    updateUserInfoUI();

    initPassengerTable();

    ui->comboTheme->clear();
    ui->comboTheme->addItem("跟随系统", (int)SettingsManager::ThemeMode::System);
    ui->comboTheme->addItem("浅色", (int)SettingsManager::ThemeMode::Light);
    ui->comboTheme->addItem("深色", (int)SettingsManager::ThemeMode::Dark);

    auto &sm = SettingsManager::instance();

    int themeIndex = ui->comboTheme->findData((int)sm.themeMode());
    if (themeIndex >= 0) ui->comboTheme->setCurrentIndex(themeIndex);

    int minScale = (int)(SettingsManager::instance().minScale * 100);
    int maxScale = (int)(SettingsManager::instance().maxScale * 100);

    ui->sliderScale->setRange(minScale, maxScale);
    ui->sliderScale->setValue((int)qRound(sm.scaleFactor() * 100.0));
    ui->lblScaleValue->setText(QString("%1%").arg(ui->sliderScale->value()));

    // 主题切换
    connect(ui->comboTheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx){
                int data = ui->comboTheme->itemData(idx).toInt();
                SettingsManager::instance().setThemeMode((SettingsManager::ThemeMode)data);
            });

    // 缩放
    connect(ui->sliderScale, &QSlider::valueChanged,
            this, [this](int v){
                ui->lblScaleValue->setText(QString("%1%").arg(v));
                SettingsManager::instance().setScaleFactor(v / 100.0);
            });

    auto *nm = NetworkManager::instance();

    connect(nm, &NetworkManager::loginStateChanged, this, [this](bool loggedIn){
        updateLoginUI();
        updateUserInfoUI();

        if (loggedIn) {
            requestPassengers();
        } else {
            m_passengers.clear();
            fillPassengerTable(m_passengers);
            if (m_passengerConn) { QObject::disconnect(m_passengerConn); m_passengerConn = {}; }
        }
    });

    connect(nm, &NetworkManager::forceLogout, this, [this](const QString&){
        // UI清理
        updateLoginUI();
        updateUserInfoUI();

        // 关闭仍在显示的弹窗，避免用户卡住
        if (m_activeDialog) m_activeDialog->reject();

        m_passengers.clear();
        fillPassengerTable(m_passengers);
        if (m_passengerConn) { QObject::disconnect(m_passengerConn); m_passengerConn = {}; }
    });

    if (nm->isLoggedIn()) requestPassengers();
}

ProfilePage::~ProfilePage()
{
    if (m_passengerConn) QObject::disconnect(m_passengerConn);
    delete ui;
}

void ProfilePage::bindForceLogoutToDialog(QDialog *dlg)
{
    if (!dlg) return;

    m_activeDialog = dlg;
    auto *nm = NetworkManager::instance();

    // 意外断线强制登出时：关闭弹窗
    connect(nm, &NetworkManager::forceLogout, dlg, [dlg](const QString&){
        if (dlg->isVisible()) dlg->reject();
    });

    connect(dlg, &QDialog::finished, this, [this](int){
        if (m_activeDialog) m_activeDialog.clear();
    });
}

void ProfilePage::updateLoginUI()
{
    if (NetworkManager::instance()->isLoggedIn()) {
        ui->btnLogin->setText("退出登录");
        ui->lblStatus->setText("已登录：" + NetworkManager::instance()->m_username);
        ui->btnRegister->setText("修改密码");
    } else {
        ui->btnLogin->setText("登录");
        ui->lblStatus->setText("未登录");
        ui->btnRegister->setText("注册");
    }
}

void ProfilePage::updateUserInfoUI()
{
    if (!NetworkManager::instance()->isLoggedIn()) {
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
    const QString phone    = NetworkManager::instance()->m_userInfo.phone;
    const QString realName = NetworkManager::instance()->m_userInfo.realName;
    const QString idCard   = NetworkManager::instance()->m_userInfo.idCard;

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
    auto *nm = NetworkManager::instance();

    // 已登录：退出登录
    if (nm->isLoggedIn()) {
        QMessageBox box(this);
        box.setWindowTitle("确认退出");
        box.setText("确定要退出登录吗？");
        if (!nm->m_username.isEmpty())
            box.setInformativeText("当前用户：" + nm->m_username);

        QPushButton *btnLogout = box.addButton("确认", QMessageBox::AcceptRole);
        QPushButton *btnCancel = box.addButton("取消", QMessageBox::RejectRole);

        box.setDefaultButton(btnCancel);
        box.exec();

        if (box.clickedButton() == btnLogout) {
            nm->logout();
            updateUserInfoUI();
            updateLoginUI();

            m_passengers.clear();
            fillPassengerTable(m_passengers);
            if (m_passengerConn) { QObject::disconnect(m_passengerConn); m_passengerConn = {}; }
        }
        return;
    }

    // 未登录：弹登录框
    auto *dlg = new LoginDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    bindForceLogoutToDialog(dlg);

    // 跳转至注册
    connect(dlg, &LoginDialog::requestRegister, this,
            [this, dlg]() {
                dlg->close();
                on_btnRegister_clicked();
            });

    // 提交登录：一次性等待 login_resp / error
    connect(dlg, &LoginDialog::loginSubmitted, this,
        [this, dlg](const QString &u, const QString &p) {
            auto *nm = NetworkManager::instance();

            auto conn = QSharedPointer<QMetaObject::Connection>::create();
            *conn = connect(nm, &NetworkManager::jsonReceived, dlg,
                [this, dlg, conn](const QJsonObject &resp) {
                    const QString type = resp.value(Protocol::KEY_TYPE).toString();
                    if (type != Protocol::TYPE_LOGIN_RESP && type != Protocol::TYPE_ERROR) return;

                    QObject::disconnect(*conn);

                    if (type == Protocol::TYPE_ERROR) {
                        const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                        QMessageBox::warning(dlg, "登录失败", msg.isEmpty() ? "账号或密码错误" : msg);
                        return;
                    }

                    const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                    const QJsonObject data = resp.value(Protocol::KEY_DATA).toObject();

                    if (!success) {
                        QMessageBox::warning(dlg, "登录失败", msg.isEmpty() ? "登录失败" : msg);
                        return;
                    }

                    // 登录成功
                    NetworkManager::instance()->setLoggedIn(true);
                    NetworkManager::instance()->m_username = data.value("username").toString();
                    NetworkManager::instance()->m_userInfo = Common::userFromJson(data);

                    updateUserInfoUI();
                    updateLoginUI();
                    requestPassengers();

                    dlg->accept();
                });

            nm->login(u, p);
        });

    dlg->exec();
}

void ProfilePage::on_btnRegister_clicked()
{
    // 未登录：注册
    if (!NetworkManager::instance()->isLoggedIn()) {
        auto *dlg = new RegisterDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        bindForceLogoutToDialog(dlg);

        connect(dlg, &RegisterDialog::registerSubmitted, this,
                [dlg](const QString &u, const QString &p,
                      const QString &phone, const QString &realName, const QString &idCard) {
                    NetworkManager::instance()->registerUser(u, p, phone, realName, idCard);
                });

        auto conn = QSharedPointer<QMetaObject::Connection>::create();
        *conn = connect(NetworkManager::instance(), &NetworkManager::jsonReceived, dlg,
            [dlg, conn](const QJsonObject &resp) {
                const QString type = resp.value(Protocol::KEY_TYPE).toString();
                if (type != Protocol::TYPE_REGISTER_RESP && type != Protocol::TYPE_ERROR) return;

                QObject::disconnect(*conn);

                if (type == Protocol::TYPE_ERROR) {
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                    QMessageBox::warning(dlg, "注册失败", msg.isEmpty() ? "注册失败" : msg);
                    return;
                }

                const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();

                if (success) {
                    QMessageBox::information(dlg, "注册成功", msg.isEmpty() ? "注册成功" : msg);
                    dlg->accept();
                } else {
                    QMessageBox::warning(dlg, "注册失败", msg.isEmpty() ? "注册失败" : msg);
                }
            });

        dlg->exec();
        return;
    }

    // 已登录：修改密码
    auto *dlg = new ChangePwdDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    bindForceLogoutToDialog(dlg);

    connect(dlg, &ChangePwdDialog::changePwdSubmitted, this,
            [this](const QString &oldPwd, const QString &newPwd) {
                NetworkManager::instance()->changePassword(NetworkManager::instance()->m_username, oldPwd, newPwd);
            });

    auto conn = QSharedPointer<QMetaObject::Connection>::create();
    *conn = connect(NetworkManager::instance(), &NetworkManager::jsonReceived, dlg,
        [dlg, conn](const QJsonObject &resp) {
            const QString type = resp.value(Protocol::KEY_TYPE).toString();
            if (type != Protocol::TYPE_CHANGE_PWD_RESP && type != Protocol::TYPE_ERROR) return;

            QObject::disconnect(*conn);

            if (type == Protocol::TYPE_ERROR) {
                const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                QMessageBox::warning(dlg, "修改失败", msg.isEmpty() ? "修改失败" : msg);
                return;
            }

            const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
            const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();

            if (success) {
                QMessageBox::information(dlg, "修改成功", msg.isEmpty() ? "密码修改成功" : msg);
                dlg->accept();
            } else {
                QMessageBox::warning(dlg, "修改失败", msg.isEmpty() ? "密码修改失败" : msg);
            }
        });

    dlg->exec();
}

void ProfilePage::on_btnChangePhone_clicked()
{
    if (!NetworkManager::instance()->isLoggedIn()) {
        QMessageBox::warning(this, "提示", "请先登录");
        return;
    }

    auto *dlg = new ChangePhoneDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    bindForceLogoutToDialog(dlg);

    const QString oldPhone = NetworkManager::instance()->m_userInfo.phone;
    dlg->setCurrentPhone(oldPhone);

    connect(dlg, &ChangePhoneDialog::phoneSubmitted, this,
            [this, dlg](const QString &newPhone) {
                QJsonObject data;
                data.insert("username", NetworkManager::instance()->m_username);
                data.insert("newPhone", newPhone);

                QJsonObject req;
                req.insert(Protocol::KEY_TYPE, Protocol::TYPE_CHANGE_PHONE);
                req.insert(Protocol::KEY_DATA, data);

                auto conn = QSharedPointer<QMetaObject::Connection>::create();
                *conn = connect(NetworkManager::instance(), &NetworkManager::jsonReceived, dlg,
                    [this, dlg, conn, newPhone](const QJsonObject &resp) {
                        const QString type = resp.value(Protocol::KEY_TYPE).toString();
                        if (type != Protocol::TYPE_CHANGE_PHONE_RESP && type != Protocol::TYPE_ERROR) return;

                        QObject::disconnect(*conn);

                        if (type == Protocol::TYPE_ERROR) {
                            const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                            QMessageBox::warning(dlg, "失败", msg.isEmpty() ? "电话号码修改失败" : msg);
                            return;
                        }

                        const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                        const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();

                        if (success) {
                            NetworkManager::instance()->m_userInfo.phone = newPhone;
                            updateUserInfoUI();
                            QMessageBox::information(this, "成功", msg.isEmpty() ? "电话号码修改成功" : msg);
                            dlg->accept();
                        } else {
                            QMessageBox::warning(dlg, "失败", msg.isEmpty() ? "电话号码修改失败" : msg);
                        }
                    });

                NetworkManager::instance()->sendJson(req);
            });

    dlg->exec();
}

void ProfilePage::on_cbShowPhone_toggled(bool)
{
    if (NetworkManager::instance()->isLoggedIn()) applyPrivacyMask();
}

void ProfilePage::on_cbShowRealName_toggled(bool)
{
    if (NetworkManager::instance()->isLoggedIn()) applyPrivacyMask();
}

void ProfilePage::on_cbShowIdCard_toggled(bool)
{
    if (NetworkManager::instance()->isLoggedIn()) applyPrivacyMask();
}

void ProfilePage::on_cbShowPassengers_toggled(bool)
{
    fillPassengerTable(m_passengers);
}

void ProfilePage::requestLogin()
{
    on_btnLogin_clicked();
}

void ProfilePage::initPassengerTable()
{
    auto *t = ui->tablePassengers;
    t->clear();
    t->setColumnCount(3);
    t->setHorizontalHeaderLabels({"ID", "姓名", "身份证号"});
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->verticalHeader()->setVisible(false);

    t->setColumnHidden(0, true);
}

void ProfilePage::requestPassengers()
{
    auto *nm = NetworkManager::instance();

    if (!nm->isLoggedIn()) {
        m_passengers.clear();
        fillPassengerTable(m_passengers);
        return;
    }

    if (m_passengerConn) QObject::disconnect(m_passengerConn);

    m_passengerConn = connect(nm, &NetworkManager::jsonReceived, this,
        [this, nm](const QJsonObject &obj) {
            const QString type = obj.value(Protocol::KEY_TYPE).toString();

            if (type == Protocol::TYPE_ERROR) {
                if (!nm->isConnected() || !nm->isLoggedIn()) {
                    QObject::disconnect(m_passengerConn);
                    m_passengerConn = {};
                    return;
                }

                QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

                if (!msg.contains("暂无")) {
                    QMessageBox::critical(this, "错误", msg);
                }

                QObject::disconnect(m_passengerConn);
                m_passengerConn = {};
                return;
            }

            if (type != Protocol::TYPE_PASSENGER_GET_RESP) return;

            const bool ok = obj.value(Protocol::KEY_SUCCESS).toBool();
            if (!ok) {
                QMessageBox::warning(this, "提示", obj.value(Protocol::KEY_MESSAGE).toString());
                QObject::disconnect(m_passengerConn);
                m_passengerConn = {};
                return;
            }

            QJsonObject data = obj.value(Protocol::KEY_DATA).toObject();
            QJsonArray arr = data.value("passengers").toArray();

            m_passengers = Common::passengersFromJsonArray(arr);
            fillPassengerTable(m_passengers);

            QObject::disconnect(m_passengerConn);
            m_passengerConn = {};
        });

    QJsonObject req;
    req.insert(Protocol::KEY_TYPE, Protocol::TYPE_PASSENGER_GET);
    req.insert(Protocol::KEY_DATA, QJsonObject());
    nm->sendJson(req);
}

void ProfilePage::fillPassengerTable(const QList<Common::PassengerInfo>& list)
{
    auto *t = ui->tablePassengers;
    t->setRowCount(0);
    t->setRowCount(list.size());

    for (int i = 0; i < list.size(); ++i) {
        const auto &p = list[i];
        QString name;
        QString idCard;
        if(ui->cbShowPassengers->isChecked()){
            name = p.name;
            idCard = p.idCard;
        }
        else{
            name = maskMiddle(p.name, 0, 1);
            idCard = maskMiddle(p.idCard, 4, 4);
        }
        t->setItem(i, 0, new QTableWidgetItem(QString::number(p.id)));
        t->setItem(i, 1, new QTableWidgetItem(name));
        t->setItem(i, 2, new QTableWidgetItem(idCard));
    }

    if (list.isEmpty()) {
        t->clearSelection();
    } else {
        t->selectRow(0);
    }
}

bool ProfilePage::validatePassengerInput(const QString& name, const QString& idCard, QString* err) const
{
    if (name.trimmed().isEmpty()) {
        if (err) *err = "姓名不能为空";
        return false;
    }
    if (idCard.trimmed().isEmpty()) {
        if (err) *err = "身份证号不能为空";
        return false;
    }
    static const QRegularExpression hasLetterOrDigit(R"([A-Za-z0-9])");
    if (hasLetterOrDigit.match(name).hasMatch()) {
        if (err) *err = "姓名不能包含字母或数字";
        return false;
    }
    static const QRegularExpression re(R"(^\d{17}(\d|X|x)$)");
    if (!re.match(idCard).hasMatch()) {
        if (err) *err = "无效的身份证号";
        return false;
    }
    if (idCard == NetworkManager::instance()->m_userInfo.idCard && name == NetworkManager::instance()->m_userInfo.realName){
        if (err) *err = "不能添加本人信息";
        return false;
    }
    return true;
}

void ProfilePage::on_btnAddPassenger_clicked()
{
    if (!NetworkManager::instance()->isLoggedIn()) {
        QMessageBox::warning(this, "提示", "请先登录");
        return;
    }

    QString name = QInputDialog::getText(this, "添加乘机人", "姓名：");
    if (name.isNull()) return;

    QString idCard = QInputDialog::getText(this, "添加乘机人", "身份证号：");
    if (idCard.isNull()) return;

    QString err;
    if (!validatePassengerInput(name, idCard, &err)) {
        QMessageBox::warning(this, "输入不合法", err);
        return;
    }

    auto *nm = NetworkManager::instance();
    if (m_passengerConn) QObject::disconnect(m_passengerConn);

    m_passengerConn = connect(nm, &NetworkManager::jsonReceived, this,
        [this, nm](const QJsonObject &obj) {
            const QString type = obj.value(Protocol::KEY_TYPE).toString();

            if (type == Protocol::TYPE_ERROR) {
                if (!nm->isConnected() || !nm->isLoggedIn()) {
                    QObject::disconnect(m_passengerConn);
                    m_passengerConn = {};
                    return;
                }
                QMessageBox::critical(this, "错误", obj.value(Protocol::KEY_MESSAGE).toString());
                QObject::disconnect(m_passengerConn);
                m_passengerConn = {};
                return;
            }
            if (type != Protocol::TYPE_PASSENGER_ADD_RESP) return;

            const bool ok = obj.value(Protocol::KEY_SUCCESS).toBool();
            const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

            if (!ok) QMessageBox::warning(this, "添加失败", msg);
            else QMessageBox::information(this, "添加成功", msg);

            QObject::disconnect(m_passengerConn);
            m_passengerConn = {};

            if (ok) requestPassengers();
        });

    QJsonObject data;
    data.insert("passenger_name", name.trimmed());
    data.insert("passenger_id_card", idCard.trimmed());

    QJsonObject req;
    req.insert(Protocol::KEY_TYPE, Protocol::TYPE_PASSENGER_ADD);
    req.insert(Protocol::KEY_DATA, data);
    nm->sendJson(req);
}

int ProfilePage::currentPassengerRow() const
{
    auto *t = ui->tablePassengers;
    const auto ranges = t->selectedRanges();
    if (ranges.isEmpty()) return -1;
    return ranges.first().topRow();
}

Common::PassengerInfo ProfilePage::passengerAtRow(int row) const
{
    Common::PassengerInfo p;
    auto *t = ui->tablePassengers;
    if (row < 0 || row >= t->rowCount()) return p;

    p.id = t->item(row, 0) ? t->item(row, 0)->text().toLongLong() : 0;
    p.name = t->item(row, 1) ? t->item(row, 1)->text() : "";
    p.idCard = t->item(row, 2) ? t->item(row, 2)->text() : "";
    return p;
}

void ProfilePage::on_btnDelPassenger_clicked()
{
    if (!NetworkManager::instance()->isLoggedIn()) {
        QMessageBox::warning(this, "提示", "请先登录");
        return;
    }

    int row = currentPassengerRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选择要删除的乘机人");
        return;
    }

    const auto p = passengerAtRow(row);

    if (QMessageBox::question(this, "确认删除",
                              QString("确定删除乘机人：%1（%2）？").arg(p.name, p.idCard))
        != QMessageBox::Yes)
    {
        return;
    }

    auto *nm = NetworkManager::instance();
    if (m_passengerConn) QObject::disconnect(m_passengerConn);

    m_passengerConn = connect(nm, &NetworkManager::jsonReceived, this,
        [this, nm](const QJsonObject &obj)
        {
            const QString type = obj.value(Protocol::KEY_TYPE).toString();

            if (type == Protocol::TYPE_ERROR) {
                if (!nm->isConnected() || !nm->isLoggedIn()) {
                    QObject::disconnect(m_passengerConn);
                    m_passengerConn = {};
                    return;
                }
                QString msg = obj.value(Protocol::KEY_MESSAGE).toString();
                qDebug() << msg;
                if(!msg.contains("暂无")) QMessageBox::critical(this, "错误", msg);
                QObject::disconnect(m_passengerConn);
                m_passengerConn = {};
                return;
            }
            if (type != Protocol::TYPE_PASSENGER_DEL_RESP) return;

            const bool ok = obj.value(Protocol::KEY_SUCCESS).toBool();
            const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

            if (!ok) QMessageBox::warning(this, "删除失败", msg);
            else QMessageBox::information(this, "删除成功", msg);

            QObject::disconnect(m_passengerConn);
            m_passengerConn = {};

            if (ok) requestPassengers();
        });

    QJsonObject data;
    data.insert("passenger_name", p.name);
    data.insert("passenger_id_card", p.idCard);

    QJsonObject req;
    req.insert(Protocol::KEY_TYPE, Protocol::TYPE_PASSENGER_DEL);
    req.insert(Protocol::KEY_DATA, data);
    nm->sendJson(req);
}


