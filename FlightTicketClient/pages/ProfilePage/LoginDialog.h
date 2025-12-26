#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    QString username() const;
    QString password() const;

signals:
    void loginSubmitted(const QString &username, const QString &password);

    void requestRegister();

private slots:
    void on_btnLogin_clicked();
    void on_btnCancel_clicked();

    void on_btnGoRegister_clicked();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
