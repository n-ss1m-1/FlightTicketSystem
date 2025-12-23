#ifndef CHANGEPHONEDIALOG_H
#define CHANGEPHONEDIALOG_H

#include <QDialog>

namespace Ui {
class ChangePhoneDialog;
}

class ChangePhoneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePhoneDialog(QWidget *parent = nullptr);
    ~ChangePhoneDialog();

    void setCurrentPhone(const QString& phone);


signals:
    void phoneSubmitted(const QString& newPhone);


private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_leNewPhone_textEdited(const QString &);


private:
    Ui::ChangePhoneDialog *ui;

    bool validatePhone();
    void updateSubmitEnabled();
};

#endif // CHANGEPHONEDIALOG_H
