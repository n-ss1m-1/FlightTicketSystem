#include "OrdersPage.h"
#include "ui_OrdersPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
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
    model->setHorizontalHeaderLabels({"订单ID", "航班ID", "乘客姓名", "状态", "下单时间"});
    ui->tableOrders->setModel(model);
    ui->tableOrders->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableOrders->horizontalHeader()->setStretchLastSection(true);

    // 【重要】连接网络信号
    connect(NetworkManager::instance(), &NetworkManager::jsonReceived,
            this, &OrdersPage::onJsonReceived);
}

OrdersPage::~OrdersPage() {
    delete ui;
}

void OrdersPage::loadOrders() {
    QJsonObject data;
    data.insert("userId", 1); // 临时固定ID，待登录功能完善后修改

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

    qint64 orderId = model->data(model->index(row, 0)).toLongLong();

    auto reply = QMessageBox::question(this, "确认取消",
                                       QString("确定要取消订单 %1 吗？").arg(orderId));
    if (reply != QMessageBox::Yes) return;

    QJsonObject data;
    data.insert("userId", 1);
    data.insert("orderId", orderId);

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_CANCEL);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

// 这里的定义必须和 .h 文件中的声明完全匹配
void OrdersPage::onJsonReceived(const QJsonObject &obj) {
    QString type = obj.value(Protocol::KEY_TYPE).toString();
    QJsonObject data = obj.value(Protocol::KEY_DATA).toObject();

    if (type == Protocol::TYPE_ORDER_LIST_RESP) {
        QJsonArray orders = data.value("orders").toArray();
        model->removeRows(0, model->rowCount());

        for (int i = 0; i < orders.size(); ++i) {
            QJsonObject o = orders[i].toObject();
            QList<QStandardItem*> rowItems;
            rowItems << new QStandardItem(QString::number(o.value("id").toVariant().toLongLong()));
            rowItems << new QStandardItem(QString::number(o.value("flightId").toVariant().toLongLong()));
            rowItems << new QStandardItem(o.value("passengerName").toString());
            rowItems << new QStandardItem(o.value("status").toString());
            rowItems << new QStandardItem(o.value("createTime").toString());
            model->appendRow(rowItems);
        }
    }
    else if (type == Protocol::TYPE_ORDER_CANCEL_RESP) {
        QMessageBox::information(this, "成功", "订单已取消");
        loadOrders(); // 成功后自动刷新列表
    }
}
