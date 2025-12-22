#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

signals:
    void registerSubmitted(const QString &username,
                           const QString &password,
                           const QString &phone,
                           const QString &realName,
                           const QString &idCard);

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

    void on_leUsername_textEdited(const QString &arg1);

    void on_lePassword_textEdited(const QString &arg1);

    void on_lePassword2_textEdited(const QString &arg1);

    void on_leRealName_textEdited(const QString &arg1);

    void on_lePhone_textEdited(const QString &arg1);

    void on_leIdCard_textEdited(const QString &arg1);

private:
    Ui::RegisterDialog *ui;

    bool validateUsername();
    bool validatePassword();
    bool validatePassword2();
    bool validatePhone();
    bool validateIdCard();
    bool validateRealName();

    void updateSubmitEnabled();
};

#endif // REGISTERDIALOG_H
