#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include <QWidget>
#include <QJsonObject>
#include "Common/Models.h"

namespace Ui {
class ProfilePage;
}

class ProfilePage : public QWidget
{
    Q_OBJECT

public:
    explicit ProfilePage(QWidget *parent = nullptr);
    ~ProfilePage();

private slots:
    void on_btnLogin_clicked();

    void on_btnRegister_clicked();

    void on_cbShowPhone_toggled(bool checked);

    void on_cbShowRealName_toggled(bool checked);

    void on_cbShowIdCard_toggled(bool checked);

    void on_btnChangePhone_clicked();

    void on_btnAddPassenger_clicked();

    void on_btnDelPassenger_clicked();

public slots:
    void requestLogin(); // 其他页面调用登录

private:
    Ui::ProfilePage *ui;

    // 移至 NetworkManager
    // bool m_loggedIn = false;
    // QString m_username;
    // QJsonObject m_userJson;

    void updateLoginUI(); // 刷新按钮/界面
    void updateUserInfoUI(); // 根据 m_userJson 填充/刷新显示
    void applyPrivacyMask(); // 根据 checkbox 显示/脱敏
    static QString maskMiddle(const QString& s, int left, int right, QChar ch='*');

    QList<Common::PassengerInfo> m_passengers;
    QMetaObject::Connection m_passengerConn; // 临时连接

    void initPassengerTable();
    void requestPassengers();                      // 拉取列表
    void fillPassengerTable(const QList<Common::PassengerInfo>& list);

    int currentPassengerRow() const;
    Common::PassengerInfo passengerAtRow(int row) const;

    bool validatePassengerInput(const QString& name, const QString& idCard, QString* err = nullptr) const;
};

#endif // PROFILEPAGE_H
