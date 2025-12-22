#include "FlightsPage.h"
#include "ui_FlightsPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h" // 必须引入协议头文件
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>

FlightsPage::FlightsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FlightsPage)
{
    ui->setupUi(this);

    model = new QStandardItemModel(this);
    // 根据服务端返回的 Common::FlightInfo 结构设置表头
    model->setHorizontalHeaderLabels({"ID", "航班号", "出发地", "目的地", "日期", "起飞时间", "票价"});
    ui->tableView->setModel(model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 监听网络数据
    connect(NetworkManager::instance(), &NetworkManager::jsonReceived, this, &FlightsPage::onJsonReceived);

    // 初始化下拉框
    ui->comboDep->addItems({"北京", "上海", "广州", "深圳", "成都", "杭州"});
    ui->comboDest->addItems({"北京", "上海", "广州", "深圳", "成都", "杭州"});
    ui->dateEdit->setDate(QDate::currentDate());
}

FlightsPage::~FlightsPage() { delete ui; }

// 发送查询请求
void FlightsPage::on_btnSearch_clicked()
{
    QJsonObject data;
    data.insert("fromCity", ui->comboDep->currentText());
    data.insert("toCity", ui->comboDest->currentText());
    data.insert("date", ui->dateEdit->date().toString(Qt::ISODate)); // 服务端要求 ISODate

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_FLIGHT_SEARCH);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

// 发送订票请求
void FlightsPage::on_btnBook_clicked()
{
    int row = ui->tableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择一个航班！");
        return;
    }

    // 从表格获取 ID (假设ID在第0列)
    qint64 flightId = model->data(model->index(row, 0)).toLongLong();

    // 服务端要求：passengerName 和 passengerIdCard
    // 这里简单弹窗询问，实际开发中可以从用户信息里读
    bool ok;
    QString name = QInputDialog::getText(this, "乘客信息", "请输入乘客姓名:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QString idCard = QInputDialog::getText(this, "乘客信息", "请输入身份证号:", QLineEdit::Normal, "", &ok);
    if (!ok || idCard.isEmpty()) return;

    QJsonObject data;
    data.insert("userId", 1); // 临时固定为1，登录功能完成后需改为实际ID
    data.insert("flightId", flightId);
    data.insert("passengerName", name);
    data.insert("passengerIdCard", idCard);

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_CREATE);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

// 处理回复
void FlightsPage::onJsonReceived(const QJsonObject &obj)
{
    QString type = obj.value(Protocol::KEY_TYPE).toString();
    QJsonObject data = obj.value(Protocol::KEY_DATA).toObject();

    // 1. 处理查询结果
    if (type == Protocol::TYPE_FLIGHT_SEARCH_RESP) {
        QJsonArray flights = data.value("flights").toArray();
        model->removeRows(0, model->rowCount()); // 清空

        for (int i = 0; i < flights.size(); ++i) {
            QJsonObject f = flights[i].toObject();
            QList<QStandardItem*> row;
            // 字段名根据 Common::flightToJson 里的定义
            row << new QStandardItem(QString::number(f.value("id").toVariant().toLongLong()));
            row << new QStandardItem(f.value("flightNo").toString());
            row << new QStandardItem(f.value("fromCity").toString());
            row << new QStandardItem(f.value("toCity").toString());
            row << new QStandardItem(f.value("date").toString());
            row << new QStandardItem(f.value("time").toString());
            row << new QStandardItem(QString::number(f.value("price").toDouble()));
            model->appendRow(row);
        }
        ui->tableView->setColumnHidden(0, true);
    }
    // 2. 处理订票结果
    else if (type == Protocol::TYPE_ORDER_CREATE_RESP) {
        QMessageBox::information(this, "成功", obj.value("message").toString());
    }
    // 3. 处理通用错误
    else if (type == Protocol::TYPE_ERROR) {
        QMessageBox::critical(this, "错误", obj.value("message").toString());
    }
}
