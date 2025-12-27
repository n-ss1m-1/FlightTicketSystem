#ifndef FLIGHTSPAGE_H
#define FLIGHTSPAGE_H

#include <QWidget>
#include <QStandardItemModel>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui { class FlightsPage; }
QT_END_NAMESPACE

class FlightsPage : public QWidget
{
    Q_OBJECT

public:
    explicit FlightsPage(QWidget *parent = nullptr);
    ~FlightsPage();

private slots:
    void on_btnSearch_clicked(); // 查询按钮
    void on_btnBook_clicked();   // 订票按钮
    // 用于监听网络数据的槽函数
    void onJsonReceived(const QJsonObject &obj);

private:
    Ui::FlightsPage *ui;
    QStandardItemModel *model;   // 数据显示模型

    qint64 m_pendingBookFlightId = -1; // 等待订票的航班ID
    bool m_waitingPassengerPick = false;  // 正在等待 passenger_get 返回

    void sendCreateOrder(qint64 flightId, const QString& name, const QString& idCard);
};

#endif // FLIGHTSPAGE_H
