#include "OrdersPage.h"
#include "ui_OrdersPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>
#include <QShowEvent> // 【新增】必须包含此头文件

OrdersPage::OrdersPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OrdersPage)
{
    ui->setupUi(this);

    // 初始化模型
    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"订单ID", "航班ID", "乘机人", "座位号", "金额", "状态", "下单时间"});
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

// 【新增】每当页面显示时（包括点击 Tab 切换回来），自动触发刷新
void OrdersPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event); // 必须调用父类逻辑

    // 只有在已登录的情况下才自动执行刷新，避免未登录时频繁弹窗提示
    if (!NetworkManager::instance()->m_username.isEmpty()) {
        qDebug() << "订单页面已显示，执行自动刷新...";
        loadOrders();
    }
}

void OrdersPage::loadOrders() {
    // 如果是因为自动刷新触发且未登录，直接返回而不弹窗
    if (NetworkManager::instance()->m_username.isEmpty()) {
        return;
    }

    QJsonObject data;
    data.insert("username", NetworkManager::instance()->m_username);

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_LIST);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
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

void OrdersPage::onJsonReceived(const QJsonObject &obj) {
    QString type = obj.value(Protocol::KEY_TYPE).toString();

    if (type == Protocol::TYPE_ERROR) {
        // QMessageBox::warning(this, "错误", obj.value(Protocol::KEY_MESSAGE).toString());
        return;
    }

    if (type == Protocol::TYPE_ORDER_LIST_RESP) {
        QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        QJsonArray orderArr = dataObj.value("orders").toArray();

        model->removeRows(0, model->rowCount());

        for (int i = 0; i < orderArr.size(); ++i) {
            QJsonObject oObj = orderArr[i].toObject();
            Common::OrderInfo ord = Common::orderFromJson(oObj);

            QList<QStandardItem*> rowItems;
            rowItems << new QStandardItem(QString::number(ord.id));
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
    else if (type == Protocol::TYPE_ORDER_CANCEL_RESP) {
        QMessageBox::information(this, "成功", "订单已取消");
        loadOrders();
    }
}
