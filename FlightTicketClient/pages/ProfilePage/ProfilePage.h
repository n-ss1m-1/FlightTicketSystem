#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include <QWidget>

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

private:
    Ui::ProfilePage *ui;

    bool m_loggedIn = false; // 登录状态
    QString m_username; // 记录用户名

    void updateLoginUI(); // 刷新按钮/界面
};

#endif // PROFILEPAGE_H
