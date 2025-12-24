#include "OrdersPage.h"
#include "ui_OrdersPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"  // 必须引入 Models.h
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>

OrdersPage::OrdersPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OrdersPage)
{
    ui->setupUi(this);

    // 初始化模型
    model = new QStandardItemModel(this);
    // 更新表头：增加座位和金额显示
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

void OrdersPage::loadOrders() {

    if (NetworkManager::instance()->m_username.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录，无法获取订单。");
        return;
    }

    QJsonObject data;
    data.insert("username", NetworkManager::instance()->m_username); // 按用户名查订单

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_LIST);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

void OrdersPage::on_btnRefresh_clicked() {
    loadOrders();
}

void OrdersPage::on_btnCancel_clicked() {
    int row = ui->tableOrders->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要取消的订单！");
        return;
    }

    // 获取订单ID
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

    // // 统一处理错误响应
    // if (type == Protocol::TYPE_ERROR) {
    //     QMessageBox::warning(this, "错误", obj.value(Protocol::KEY_MESSAGE).toString());
    //     return;
    // }

    // 处理订单列表回复
    if (type == Protocol::TYPE_ORDER_LIST_RESP) {
        QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        QJsonArray orderArr = dataObj.value("orders").toArray();

        model->removeRows(0, model->rowCount());

        for (int i = 0; i < orderArr.size(); ++i) {
            QJsonObject oObj = orderArr[i].toObject();

            // 使用 models.h 的解析工具
            Common::OrderInfo ord = Common::orderFromJson(oObj);

            QList<QStandardItem*> rowItems;
            // 1. 订单ID
            rowItems << new QStandardItem(QString::number(ord.id));
            // 2. 航班ID
            rowItems << new QStandardItem(QString::number(ord.flightId));
            // 3. 乘客姓名
            rowItems << new QStandardItem(ord.passengerName);
            // 4. 座位号
            rowItems << new QStandardItem(ord.seatNum.isEmpty() ? "未分配" : ord.seatNum);

            // 5. 金额（分转元）
            double priceYuan = ord.priceCents / 100.0;
            rowItems << new QStandardItem(QString("￥%1").arg(QString::number(priceYuan, 'f', 2)));

            // 6. 状态转换
            QString statusText;
            switch (ord.status) {
            case Common::OrderStatus::Booked:   statusText = "已预订"; break;
            case Common::OrderStatus::Canceled: statusText = "已退票"; break;
            case Common::OrderStatus::Finished: statusText = "已完成"; break;
            default: statusText = "未知"; break;
            }
            rowItems << new QStandardItem(statusText);

            // 7. 下单时间格式化
            rowItems << new QStandardItem(ord.createdTime.toString("yyyy-MM-dd HH:mm"));

            model->appendRow(rowItems);
        }
    }
    // 处理取消订单回复
    else if (type == Protocol::TYPE_ORDER_CANCEL_RESP) {
        QMessageBox::information(this, "成功", "订单已取消");
        loadOrders(); // 刷新列表
    }
}
