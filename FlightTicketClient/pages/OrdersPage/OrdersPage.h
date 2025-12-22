#ifndef ORDERSPAGE_H
#define ORDERSPAGE_H

#include <QWidget>
#include <QStandardItemModel>
#include <QJsonObject>

namespace Ui { class OrdersPage; }

class OrdersPage : public QWidget
{
    Q_OBJECT

public:
    explicit OrdersPage(QWidget *parent = nullptr);
    ~OrdersPage();

    void loadOrders(); // 公有方法：供外部调用刷新数据

private slots:
    void on_btnRefresh_clicked();
    void on_btnCancel_clicked();
    void onJsonReceived(const QJsonObject &obj);

private:
    Ui::OrdersPage *ui;
    QStandardItemModel *model;
};

#endif // ORDERSPAGE_H
