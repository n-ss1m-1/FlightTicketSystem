#ifndef ORDERSPAGE_H
#define ORDERSPAGE_H

#include <QWidget>
#include <QStandardItemModel>
#include <QJsonObject>
#include "Common/Models.h"

namespace Ui { class OrdersPage; }

class OrdersPage : public QWidget
{
    Q_OBJECT

public:
    explicit OrdersPage(QWidget *parent = nullptr);
    ~OrdersPage();

    void loadOrders(); // 公有刷新方法

protected:
    // 重写显示事件，实现自动刷新
    void showEvent(QShowEvent *event) override;

private slots:
    void on_btnRefresh_clicked();
    void on_btnCancel_clicked();
    void onJsonReceived(const QJsonObject &obj);

private:
    Ui::OrdersPage *ui;
    QStandardItemModel *model;

    struct CachedOrder {
        Common::OrderInfo ord;
        bool fromUser = false;   // 来自按用户的查询
        bool fromOther = false;  // 来自按实名信息匹配的查询
    };

    bool m_fetchingOrders = false;
    int  m_pendingOrderListResp = 0; // 等待list响应数
    QHash<qint64, CachedOrder> m_orderCache; // key=orderId 去重

    void mergeOrdersFromArray(const QJsonArray &arr, bool fromUserList); // 合并两种订单
    void rebuildTableFromCache();
};

#endif // ORDERSPAGE_H
