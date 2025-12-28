#include "OrdersPage.h"
#include "ui_OrdersPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QDebug>
#include <QShowEvent>
#include <algorithm>

static const int ROLE_ORDER_ID = Qt::UserRole + 1;

static QString statusToText(Common::OrderStatus s)
{
    switch (s) {
    case Common::OrderStatus::Booked:   return "已预订";
    case Common::OrderStatus::Paid:     return "已支付";
    case Common::OrderStatus::Canceled: return "已退票";
    case Common::OrderStatus::Finished: return "已完成";
    default: return "未知";
    }
}

static QString dtToText(const QDateTime &dt)
{
    if (!dt.isValid()) return "--";
    return dt.toString("yyyy-MM-dd HH:mm");
}

OrdersPage::OrdersPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OrdersPage)
{
    ui->setupUi(this);

    // 初始化模型
    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({
        "订单来源", "航班号", "航线", "起飞时间", "到达时间",
        "乘机人", "座位号", "金额", "状态"
    });
    ui->tableOrders->setModel(model);
    ui->tableOrders->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableOrders->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableOrders->horizontalHeader()->setStretchLastSection(true);

    // 双击行弹出详细信息
    connect(ui->tableOrders, &QTableView::doubleClicked, this, [this](const QModelIndex &index){
        int row = index.row();
        if (row < 0) return;

        QStandardItem *it0 = model->item(row, 0);
        if (!it0) return;

        qint64 orderId = it0->data(ROLE_ORDER_ID).toLongLong();
        if (!m_orderCache.contains(orderId)) return;

        const auto &e = m_orderCache[orderId];
        const auto &ord = e.ord;
        const auto &flt = e.flight;

        const QString sourceText = e.fromUser ? "本用户" : "其他用户";

        QString detail = QString(
                             "【订单信息】\n"
                             "订单号：%1\n"
                             "下单时间：%2\n"
                             "订单来源：%3\n"
                             "下单用户ID：%4\n"
                             "状态：%5\n"
                             "金额：￥%6\n"
                             "\n"
                             "【乘机人信息】\n"
                             "姓名：%7\n"
                             "证件号：%8\n"
                             "座位号：%9\n"
                             "\n"
                             "【航班信息】\n"
                             "航班ID：%10\n"
                             "航班号：%11\n"
                             "航线：%12 → %13\n"
                             "起飞：%14\n"
                             "到达：%15\n"
                             )
                             .arg(ord.id)
                             .arg(dtToText(ord.createdTime))
                             .arg(sourceText)
                             .arg(ord.userId)
                             .arg(statusToText(ord.status))
                             .arg(QString::number(ord.priceCents / 100.0, 'f', 2))
                             .arg(ord.passengerName.isEmpty() ? "--" : ord.passengerName)
                             .arg(ord.passengerIdCard.isEmpty() ? "--" : ord.passengerIdCard)
                             .arg(ord.seatNum.isEmpty() ? "未分配" : ord.seatNum)
                             .arg(ord.flightId)
                             .arg(flt.flightNo.isEmpty() ? "--" : flt.flightNo)
                             .arg(flt.fromCity.isEmpty() ? "--" : flt.fromCity)
                             .arg(flt.toCity.isEmpty() ? "--" : flt.toCity)
                             .arg(dtToText(flt.departTime))
                             .arg(dtToText(flt.arriveTime));

        QMessageBox::information(this, "订单详情", detail);
    });

    // 连接网络信号
    connect(NetworkManager::instance(), &NetworkManager::jsonReceived,
            this, &OrdersPage::onJsonReceived);
}

OrdersPage::~OrdersPage() {
    delete ui;
}

void OrdersPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (!NetworkManager::instance()->m_username.isEmpty()) {
        qDebug() << "订单页面已显示，执行自动刷新.";
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
        if (!v.isObject()) continue;

        const QJsonObject compositeObj = v.toObject();
        const QJsonObject orderObj = compositeObj.value("order").toObject();
        const QJsonObject flightObj = compositeObj.value("flight").toObject();

        Common::OrderInfo ord = Common::orderFromJson(orderObj);
        Common::FlightInfo flt = Common::flightFromJson(flightObj);

        auto &entry = m_orderCache[ord.id]; // 若不存在会创建默认 CachedOrder
        entry.ord = ord;
        entry.flight = flt;

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
        const auto &flt = e.flight;

        const QString sourceText = e.fromUser ? "本用户" : "其他用户";

        const QString flightNo = flt.flightNo.isEmpty() ? "--" : flt.flightNo;

        QString route = "--";
        if (!flt.fromCity.isEmpty() || !flt.toCity.isEmpty()) {
            route = QString("%1 → %2")
                        .arg(flt.fromCity.isEmpty() ? "--" : flt.fromCity)
                        .arg(flt.toCity.isEmpty() ? "--" : flt.toCity);
        }

        const QString departText = dtToText(flt.departTime);
        const QString arriveText = dtToText(flt.arriveTime);

        const QString passengerText = ord.passengerName.isEmpty() ? "--" : ord.passengerName;
        const QString seatText = ord.seatNum.isEmpty() ? "未分配" : ord.seatNum;

        const double priceYuan = ord.priceCents / 100.0;
        const QString priceText = QString("￥%1").arg(QString::number(priceYuan, 'f', 2));

        const QString statusText = statusToText(ord.status);

        QList<QStandardItem*> rowItems;
        QStandardItem *itSource = new QStandardItem(sourceText);
        itSource->setData(orderIdVariant(ord.id), ROLE_ORDER_ID); // orderId 不显示，但绑定到第一列 item

        rowItems << itSource;
        rowItems << new QStandardItem(flightNo);
        rowItems << new QStandardItem(route);
        rowItems << new QStandardItem(departText);
        rowItems << new QStandardItem(arriveText);
        rowItems << new QStandardItem(passengerText);
        rowItems << new QStandardItem(seatText);
        rowItems << new QStandardItem(priceText);
        rowItems << new QStandardItem(statusText);

        model->appendRow(rowItems);
    }
}

QVariant OrdersPage::orderIdVariant(qint64 id) const
{
    return QVariant::fromValue<qint64>(id);
}

void OrdersPage::on_btnRefresh_clicked() {
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

    // orderId从第一列item的UserRole取
    QStandardItem *it0 = model->item(row, 0);
    if (!it0) return;

    qint64 orderId = it0->data(ROLE_ORDER_ID).toLongLong();
    if (orderId <= 0) {
        QMessageBox::warning(this, "提示", "订单ID无效，无法取消。");
        return;
    }

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
        const QJsonArray orderArr = dataObj.value("ordersAndflights").toArray();

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
