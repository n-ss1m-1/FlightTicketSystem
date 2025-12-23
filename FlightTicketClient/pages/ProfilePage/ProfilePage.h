#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include <QWidget>
#include <QJsonObject>

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

private:
    Ui::ProfilePage *ui;

    bool m_loggedIn = false; // 登录状态
    // QString m_username; // 移至 NetworkManager
    QJsonObject m_userJson; // 记录用户信息

    void updateLoginUI(); // 刷新按钮/界面
    void updateUserInfoUI(); // 根据 m_userJson 填充/刷新显示
    void applyPrivacyMask(); // 根据 checkbox 显示/脱敏
    static QString maskMiddle(const QString& s, int left, int right, QChar ch='*');
};

#endif // PROFILEPAGE_H
