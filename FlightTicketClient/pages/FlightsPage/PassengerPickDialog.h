#ifndef PASSENGERPICKDIALOG_H
#define PASSENGERPICKDIALOG_H

#include <QDialog>
#include "Common/Models.h"
#include <QStandardItemModel>

namespace Ui {
class PassengerPickDialog;
}

class PassengerPickDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PassengerPickDialog(const Common::PassengerInfo& self,
                                 const QList<Common::PassengerInfo>& others,
                                 const Common::FlightInfo& flight,
                                 QWidget *parent = nullptr);
    ~PassengerPickDialog();

    Common::PassengerInfo selectedPassenger() const;

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

private:
    Ui::PassengerPickDialog *ui;
    QStandardItemModel *m_model = nullptr;

    QList<Common::PassengerInfo> m_all;
    int m_selectedRow = -1;

    static QString maskIdCard(const QString& id);
    void initTable();
    void loadData();

    Common::FlightInfo m_flight; // 航班信息缓存
    void updateFlightInfoUi();
};

#endif // PASSENGERPICKDIALOG_H
