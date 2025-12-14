#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QJsonObject>

class QLineEdit;
class QPushButton;
class QLabel;

class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);

private slots:
    void onLoginClicked();
    void onConnected();
    void onDisconnected();
    void onJsonReceived(const QJsonObject &obj);
    void onError(const QString &msg);

private:
    void setupUi();
    void sendLoginRequest();

private:
    QLineEdit *m_editUser = nullptr;
    QLineEdit *m_editPass = nullptr;
    QPushButton *m_btnLogin = nullptr;
    QLabel *m_labelStatus = nullptr;
};

#endif // LOGINWINDOW_H

