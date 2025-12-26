#include "ProfilePage.h"
#include "ui_ProfilePage.h"
#include "LoginDialog.h"
#include <QMessageBox>
#include "NetworkManager.h"
#include "RegisterDialog.h"
#include "ChangePwdDialog.h"
#include "ChangePhoneDialog.h"
#include "SettingsManager.h"
#include <QLineEdit>
#include <QInputDialog>

ProfilePage::ProfilePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProfilePage)
{
    ui->setupUi(this);

    updateLoginUI();

    initPassengerTable();

    connect(ui->btnAddPassenger, &QPushButton::clicked, this, &ProfilePage::on_btnAddPassenger_clicked);
    connect(ui->btnDelPassenger, &QPushButton::clicked, this, &ProfilePage::on_btnDelPassenger_clicked);


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
}

ProfilePage::~ProfilePage()
{
    if (m_passengerConn) QObject::disconnect(m_passengerConn);
    delete ui;
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
    if (NetworkManager::instance()->isLoggedIn()) {
        QMessageBox box(this);
        box.setWindowTitle("确认退出");
        box.setText("确定要退出登录吗？");
        if (!NetworkManager::instance()->m_username.isEmpty())
            box.setInformativeText("当前用户：" + NetworkManager::instance()->m_username);

        QPushButton *btnLogout = box.addButton("确认", QMessageBox::AcceptRole);
        QPushButton *btnCancel = box.addButton("取消", QMessageBox::RejectRole);

        box.setDefaultButton(btnCancel);
        box.exec();

        if (box.clickedButton() == btnLogout) {
            ui->btnLogin->setEnabled(false);

            QJsonObject req;
            req.insert(Protocol::KEY_TYPE, Protocol::TYPE_LOGOUT);

            QJsonObject data;
            data.insert("username", NetworkManager::instance()->m_username);
            req.insert(Protocol::KEY_DATA, data);

            auto *nm = NetworkManager::instance();
            auto *conn = new QMetaObject::Connection;
            *conn = connect(nm, &NetworkManager::jsonReceived, this,
                [this, conn](const QJsonObject &resp) {
                    const QString type = resp.value(Protocol::KEY_TYPE).toString();

                    if (type != Protocol::TYPE_LOGOUT_RESP && type != Protocol::TYPE_ERROR)
                        return;

                    disconnect(*conn);
                    delete conn;

                    const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();

                    if (type == Protocol::TYPE_LOGOUT_RESP && success) {
                        NetworkManager::instance()->setLoggedIn(false);
                        NetworkManager::instance()->m_username.clear();
                        m_userJson = QJsonObject();
                        updateUserInfoUI();
                        updateLoginUI();
                        requestPassengers();
                    } else {
                        // 失败保持当前登录
                        QMessageBox::warning(this, "退出登录失败", msg.isEmpty() ? "退出登录失败" : msg);
                    }
                    ui->btnLogin->setEnabled(true);
                });

            nm->sendJson(req);
        }
        return;
    }

    auto *dlg = new LoginDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    // 跳转至注册
    connect(dlg, &LoginDialog::requestRegister, this,
            [this, dlg]() {
                dlg->close();
                on_btnRegister_clicked();
            });

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
                        NetworkManager::instance()->setLoggedIn(true);

                        NetworkManager::instance()->m_username = data.value("username").toString();

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
    if (!NetworkManager::instance()->isLoggedIn()) {
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
                NetworkManager::instance()->changePassword(NetworkManager::instance()->m_username, oldPwd, newPwd);
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

void ProfilePage::on_btnChangePhone_clicked()
{
    if (!NetworkManager::instance()->isLoggedIn()) {
        QMessageBox::warning(this, "提示", "请先登录");
        return;
    }

    auto *dlg = new ChangePhoneDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);


    const QString oldPhone = m_userJson.value("phone").toString();
    dlg->setCurrentPhone(oldPhone);

    connect(dlg, &ChangePhoneDialog::phoneSubmitted, this,
            [this](const QString &newPhone) {

                QJsonObject data;
                data.insert("username", NetworkManager::instance()->m_username);
                data.insert("newPhone", newPhone);

                QJsonObject req;
                req.insert(Protocol::KEY_TYPE, Protocol::TYPE_CHANGE_PHONE);
                req.insert(Protocol::KEY_DATA, data);

                NetworkManager::instance()->sendJson(req);
            });


    connect(NetworkManager::instance(), &NetworkManager::jsonReceived, dlg,
            [this, dlg](const QJsonObject &resp) {

                const QString type = resp.value(Protocol::KEY_TYPE).toString();


                if (type == Protocol::TYPE_CHANGE_PHONE_RESP) {
                    const bool success = resp.value(Protocol::KEY_SUCCESS).toBool();
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();

                    if (success) {

                        const QString newPhone = dlg->findChild<QLineEdit*>("leNewPhone")->text().trimmed();
                        m_userJson.insert("phone", newPhone);

                        updateUserInfoUI();

                        QMessageBox::information(this, "成功", msg.isEmpty() ? "电话号码修改成功" : msg);
                        dlg->accept();
                    } else {
                        QMessageBox::warning(dlg, "失败", msg.isEmpty() ? "电话号码修改失败" : msg);
                    }
                    return;
                }

                if (type == Protocol::TYPE_ERROR) {
                    const QString msg = resp.value(Protocol::KEY_MESSAGE).toString();
                    QMessageBox::warning(dlg, "失败", msg.isEmpty() ? "电话号码修改失败" : msg);
                    return;
                }
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

    if (m_passengerConn) QObject::disconnect(m_passengerConn);

    m_passengerConn = connect(nm, &NetworkManager::jsonReceived, this,
            [this](const QJsonObject &obj) {
                const QString type = obj.value(Protocol::KEY_TYPE).toString();
                if (type == Protocol::TYPE_ERROR) {
                    QMessageBox::critical(this, "错误", obj.value(Protocol::KEY_MESSAGE).toString());
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

        t->setItem(i, 0, new QTableWidgetItem(QString::number(p.id)));
        t->setItem(i, 1, new QTableWidgetItem(p.name));
        t->setItem(i, 2, new QTableWidgetItem(p.idCard));
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
    // TODO
    return true;
}

void ProfilePage::on_btnAddPassenger_clicked()
{
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
        [this](const QJsonObject &obj) {
            const QString type = obj.value(Protocol::KEY_TYPE).toString();

            if (type == Protocol::TYPE_ERROR) {
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
                              [this](const QJsonObject &obj)
                              {
                                  const QString type = obj.value(Protocol::KEY_TYPE).toString();

                                  if (type == Protocol::TYPE_ERROR) {
                                      QMessageBox::critical(this, "错误", obj.value(Protocol::KEY_MESSAGE).toString());
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

