#ifndef ADDUSERDIALOG_H
#define ADDUSERDIALOG_H
#include <QDialog>

namespace Ui { class AddUserDialog; }

class AddUserDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddUserDialog(QWidget *parent = nullptr);
    ~AddUserDialog();
private slots:
    void on_btnConfirm_clicked();
    void on_btnCancel_clicked();
private:
    Ui::AddUserDialog *ui;
};
#endif // ADDUSERDIALOG_H
