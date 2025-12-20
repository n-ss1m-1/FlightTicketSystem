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

private:
    Ui::RegisterDialog *ui;
};

#endif // REGISTERDIALOG_H
