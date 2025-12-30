#ifndef RESCHEDULEDIALOG_H
#define RESCHEDULEDIALOG_H

#include <QDialog>
#include "Common/Models.h"
#include <QStandardItemModel>

namespace Ui {
class RescheduleDialog;
}

class RescheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RescheduleDialog(QWidget *parent = nullptr);
    ~RescheduleDialog();

    void setData(const Common::OrderInfo &oriOrder, const Common::FlightInfo &oriFlight);

signals:
    void rescheduled(const Common::OrderInfo &newOrder,
                     const Common::FlightInfo &newFlight,
                     qint32 priceDif);

private slots:
    void on_btnSearch_clicked();
    void on_btnConfirm_clicked();

private:
    Ui::RescheduleDialog *ui;

    Common::OrderInfo  m_oriOrder;
    Common::FlightInfo m_oriFlight;

    QList<Common::FlightInfo> m_flights;
    QStandardItemModel *m_model = nullptr;

    void sendFlightSearch();
    void sendReschedule(qint64 newFlightId);

    QMetaObject::Connection m_searchConn;
    bool m_waitingSearch = false;

};

#endif // RESCHEDULEDIALOG_H
