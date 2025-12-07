#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#endif // LOGINWINDOW_H
#pragma once

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
    QLineEdit *m_editHost;
    QLineEdit *m_editPort;
    QLineEdit *m_editUser;
    QLineEdit *m_editPass;
    QPushButton *m_btnLogin;
    QLabel *m_labelStatus;
};
