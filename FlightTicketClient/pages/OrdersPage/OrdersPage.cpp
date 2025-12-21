#include "OrdersPage.h"
#include "ui_OrdersPage.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

OrdersPage::OrdersPage(QWidget *parent) : QWidget(parent), ui(new Ui::OrdersPage) {
    ui->setupUi(this);
    model = new QSqlQueryModel(this);
    loadOrders();
}

OrdersPage::~OrdersPage() { delete ui; }

// 显示订单详情
void OrdersPage::loadOrders() {
    // 从 orders 表查出订单，从 flights 表查出对应的航班号和路线
    QString sql = "SELECT o.order_id AS '订单号', f.flight_no AS '航班号', "
                  "f.departure AS '出发地', f.destination AS '目的地', "
                  "o.order_time AS '下单时间', o.status AS '状态' "
                  "FROM orders o JOIN flights f ON o.flight_id = f.id";

    model->setQuery(sql);
    ui->tableOrders->setModel(model);


    ui->tableOrders->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableOrders->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableOrders->horizontalHeader()->setStretchLastSection(true);
}

void OrdersPage::on_btnRefresh_clicked() {
    loadOrders();
}

// 仅删除订单记录
void OrdersPage::on_btnCancel_clicked() {
    // 获取选中的行
    int row = ui->tableOrders->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要取消的订单！");
        return;
    }

    // 获取订单号 (第一列 order_id)
    QString orderId = model->data(model->index(row, 0)).toString();

    // 弹窗
    auto reply = QMessageBox::question(this, "确认取消",
                                       QString("确定要取消订单 %1 吗？").arg(orderId));
    if (reply != QMessageBox::Yes) return;

    // 执行删除操作
    QSqlQuery query;
    query.prepare("DELETE FROM orders WHERE order_id = :oid");
    query.bindValue(":oid", orderId);

    if (query.exec()) {
        QMessageBox::information(this, "成功", "订单已成功取消。");
        loadOrders(); // 刷新表格显示
    } else {
        QMessageBox::critical(this, "错误", "取消失败：" + query.lastError().text());
    }
}
