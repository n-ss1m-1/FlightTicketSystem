#include "OrdersPage.h"
#include "ui_OrdersPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>
#include <QShowEvent>

OrdersPage::OrdersPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OrdersPage)
{
    ui->setupUi(this);

    // 初始化模型
    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"订单ID", "订单来源", "航班ID", "乘机人", "座位号", "金额", "状态", "下单时间"});
    ui->tableOrders->setModel(model);
    ui->tableOrders->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableOrders->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableOrders->horizontalHeader()->setStretchLastSection(true);

    // 连接网络信号
    connect(NetworkManager::instance(), &NetworkManager::jsonReceived,
            this, &OrdersPage::onJsonReceived);

}

OrdersPage::~OrdersPage() {
    delete ui;
}

// 每当页面显示时（包括点击 Tab 切换回来），自动触发刷新
void OrdersPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event); // 必须调用父类逻辑

    // 只有在已登录的情况下才自动执行刷新，避免未登录时频繁弹窗提示
    if (!NetworkManager::instance()->m_username.isEmpty()) {
        qDebug() << "订单页面已显示，执行自动刷新...";
        loadOrders();
    }
}

void OrdersPage::loadOrders()
{
    if (NetworkManager::instance()->m_username.isEmpty()) {
        return;
    }

    m_fetchingOrders = true;
    m_pendingOrderListResp = 2;   // 期待两个响应：order_list_response + order_list_my_response
    m_orderCache.clear();
    model->removeRows(0, model->rowCount());

    auto sendListReq = [&](const QString &reqType) {
        QJsonObject root;
        root.insert(Protocol::KEY_TYPE, reqType);
        root.insert(Protocol::KEY_DATA, QJsonObject{});
        NetworkManager::instance()->sendJson(root);
    };

    sendListReq(Protocol::TYPE_ORDER_LIST);
    sendListReq(Protocol::TYPE_ORDER_LIST_MY);
}

void OrdersPage::mergeOrdersFromArray(const QJsonArray &arr, bool fromUserList)
{
    for (const auto &v : arr) {
        const QJsonObject oObj = v.toObject();
        Common::OrderInfo ord = Common::orderFromJson(oObj);

        auto &entry = m_orderCache[ord.id]; // 若不存在会创建默认 CachedOrder
        entry.ord = ord;                    // 覆盖/更新订单内容

        if (fromUserList) entry.fromUser = true;
        else              entry.fromOther = true;
    }
}


void OrdersPage::rebuildTableFromCache()
{
    QList<CachedOrder> list = m_orderCache.values();

    std::sort(list.begin(), list.end(), [](const CachedOrder &a, const CachedOrder &b){
        return a.ord.createdTime > b.ord.createdTime;
    });

    model->removeRows(0, model->rowCount());

    for (const auto &e : list) {
        const auto &ord = e.ord;

        const QString sourceText = e.fromUser ? "本用户" : "其他用户";

        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(ord.id));
        rowItems << new QStandardItem(sourceText);
        rowItems << new QStandardItem(QString::number(ord.flightId));
        rowItems << new QStandardItem(ord.passengerName);
        rowItems << new QStandardItem(ord.seatNum.isEmpty() ? "未分配" : ord.seatNum);

        double priceYuan = ord.priceCents / 100.0;
        rowItems << new QStandardItem(QString("￥%1").arg(QString::number(priceYuan, 'f', 2)));

        QString statusText;
        switch (ord.status) {
        case Common::OrderStatus::Booked:   statusText = "已预订"; break;
        case Common::OrderStatus::Canceled: statusText = "已退票"; break;
        case Common::OrderStatus::Finished: statusText = "已完成"; break;
        default: statusText = "未知"; break;
        }
        rowItems << new QStandardItem(statusText);
        rowItems << new QStandardItem(ord.createdTime.toString("yyyy-MM-dd HH:mm"));

        model->appendRow(rowItems);
    }
}



void OrdersPage::on_btnRefresh_clicked() {
    // 手动点击刷新按钮时，如果没有登录，则予以提示
    if (NetworkManager::instance()->m_username.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录，无法获取订单。");
        return;
    }
    loadOrders();
}

void OrdersPage::on_btnCancel_clicked() {
    int row = ui->tableOrders->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要取消的订单！");
        return;
    }

    qint64 orderId = model->data(model->index(row, 0)).toLongLong();

    auto reply = QMessageBox::question(this, "确认取消",
                                       QString("确定要取消订单 %1 吗？").arg(orderId));
    if (reply != QMessageBox::Yes) return;

    QJsonObject data;
    data.insert("username", NetworkManager::instance()->m_username);
    data.insert("orderId", orderId);

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_CANCEL);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

void OrdersPage::onJsonReceived(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    qDebug() << "==== [服务器返回的原始数据] ====";
    qDebug() << doc.toJson(QJsonDocument::Indented);
    qDebug() << "================================";

    const QString type = obj.value(Protocol::KEY_TYPE).toString();

    if (type == Protocol::TYPE_ERROR) {
        if (m_fetchingOrders && m_pendingOrderListResp > 0) {
            m_pendingOrderListResp--;
            if (m_pendingOrderListResp == 0) {
                m_fetchingOrders = false;
                rebuildTableFromCache();
            }
        }
        return;
    }

    // 两种订单列表响应：合并去重
    if (type == Protocol::TYPE_ORDER_LIST_RESP || type == Protocol::TYPE_ORDER_LIST_MY_RESP) {
        const QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        const QJsonArray orderArr = dataObj.value("orders").toArray();

        const bool fromUserList = (type == Protocol::TYPE_ORDER_LIST_RESP);
        mergeOrdersFromArray(orderArr, fromUserList);

        if (m_fetchingOrders && m_pendingOrderListResp > 0) {
            m_pendingOrderListResp--;
            if (m_pendingOrderListResp == 0) {
                m_fetchingOrders = false;
                rebuildTableFromCache();
            }
        } else {
            rebuildTableFromCache();
        }
        return;
    }


    if (type == Protocol::TYPE_ORDER_CANCEL_RESP) {
        QMessageBox::information(this, "成功", "订单已取消");
        loadOrders();
        return;
    }
}
