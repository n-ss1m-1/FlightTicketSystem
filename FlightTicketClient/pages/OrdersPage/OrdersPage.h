#ifndef ORDERSPAGE_H
#define ORDERSPAGE_H

#include <QWidget>
#include <QSqlQueryModel>

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

private:
    Ui::OrdersPage *ui;
    QSqlQueryModel *model;
};
#endif
