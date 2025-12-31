#ifndef PASSENGERPICKDIALOG_H
#define PASSENGERPICKDIALOG_H

#include <QDialog>
#include "Common/Models.h"
#include <QStandardItemModel>
#include "../ProfilePage/ProfilePage.h"

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
                                 ProfilePage* profilePage,
                                 QWidget *parent = nullptr);
    ~PassengerPickDialog();

    Common::PassengerInfo selectedPassenger() const;

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();
    void on_btnAddPassenger_clicked();

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

    void requestPassengers(); // 拉取常用乘机人并刷新表格
    void reloadAll(const QList<Common::PassengerInfo>& others);

    Common::PassengerInfo m_self; // 保存本用户（保证永远是第一行）

    QMetaObject::Connection m_getConn;
    QMetaObject::Connection m_addConn;

    ProfilePage* m_profilePage;
};

#endif // PASSENGERPICKDIALOG_H
