#ifndef ADDPASSENGERDIALOG_H
#define ADDPASSENGERDIALOG_H

#include <QDialog>

namespace Ui {
class AddPassengerDialog;
}

class AddPassengerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPassengerDialog(QWidget *parent = nullptr);
    ~AddPassengerDialog();


signals:
    void passengerSubmitted(const QString& name, const QString& idCard);

private slots:
    void on_btnCancel_clicked();
    void on_btnOk_clicked();

    void on_leName_textEdited(const QString &);
    void on_leIdCard_textEdited(const QString &);

private:
    Ui::AddPassengerDialog *ui;

    bool validateName();
    bool validateIdCard();
    void updateSubmitEnabled();

};

#endif // ADDPASSENGERDIALOG_H
