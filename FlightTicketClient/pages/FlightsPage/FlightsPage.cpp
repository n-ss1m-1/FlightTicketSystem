#include "FlightsPage.h"
#include "ui_FlightsPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>

FlightsPage::FlightsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FlightsPage)
{
    ui->setupUi(this);

    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"ID", "航班号", "出发地", "目的地", "起飞时间", "到达时间", "票价"});
    ui->tableView->setModel(model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(NetworkManager::instance(), &NetworkManager::jsonReceived, this, &FlightsPage::onJsonReceived);

    ui->comboDep->addItems({"北京", "上海", "广州", "深圳", "成都", "杭州"});
    ui->comboDest->addItems({"北京", "上海", "广州", "深圳", "成都", "杭州"});
    ui->dateEdit->setDate(QDate::currentDate());
}

FlightsPage::~FlightsPage() { delete ui; }

void FlightsPage::on_btnSearch_clicked()
{
    QJsonObject data;
    data.insert("fromCity", ui->comboDep->currentText());
    data.insert("toCity", ui->comboDest->currentText());
    data.insert("date", ui->dateEdit->date().toString(Qt::ISODate));

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_FLIGHT_SEARCH);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

void FlightsPage::on_btnBook_clicked()
{
    if (NetworkManager::instance()->m_username == "") {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }

    int row = ui->tableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择一个航班！");
        return;
    }

    qint64 flightId = model->data(model->index(row, 0)).toLongLong();

    bool ok;
    QString name = QInputDialog::getText(this, "乘客信息", "请输入乘客姓名:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QString idCard = QInputDialog::getText(this, "乘客信息", "请输入身份证号:", QLineEdit::Normal, "", &ok);
    if (!ok || idCard.isEmpty()) return;

    QJsonObject data;
    data.insert("username", NetworkManager::instance()->m_username);
    data.insert("flightId", flightId);
    data.insert("passengerName", name);
    data.insert("passengerIdCard", idCard);

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_ORDER_CREATE);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

void FlightsPage::onJsonReceived(const QJsonObject &obj)
{
    QString type = obj.value(Protocol::KEY_TYPE).toString();

    // 1. 恢复错误处理逻辑
    if (type == Protocol::TYPE_ERROR) {
        // 如果出错（比如服务端返回查询失败），也建议清空旧数据
        // model->removeRows(0, model->rowCount());
        // QMessageBox::warning(this, "查询提示", obj.value(Protocol::KEY_MESSAGE).toString());
        return;
    }

    // 2. 处理查询结果
    if (type == Protocol::TYPE_FLIGHT_SEARCH_RESP) {
        QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        QJsonArray flightsArr = dataObj.value("flights").toArray();

        // 无论有没有查到，只要收到查询响应，立即清空旧数据
        model->removeRows(0, model->rowCount());

        // 判断结果是否为空
        if (flightsArr.isEmpty()) {
            QMessageBox::information(this, "查询结果", "抱歉，未找到符合条件的航班。");
            return;
        }

        for (int i = 0; i < flightsArr.size(); ++i) {
            QJsonObject fObj = flightsArr[i].toObject();
            Common::FlightInfo f = Common::flightFromJson(fObj);

            QList<QStandardItem*> row;
            row << new QStandardItem(QString::number(f.id));
            row << new QStandardItem(f.flightNo);
            row << new QStandardItem(f.fromCity);
            row << new QStandardItem(f.toCity);
            row << new QStandardItem(f.departTime.toString("yyyy-MM-dd HH:mm"));
            row << new QStandardItem(f.arriveTime.toString("yyyy-MM-dd HH:mm"));

            double priceYuan = f.priceCents / 100.0;
            row << new QStandardItem(QString("￥%1").arg(QString::number(priceYuan, 'f', 2)));

            model->appendRow(row);
        }
        ui->tableView->setColumnHidden(0, true);
    }
    // 处理订票结果
    else if (type == Protocol::TYPE_ORDER_CREATE_RESP) {
        QString msg = obj.value(Protocol::KEY_MESSAGE).toString();
        QMessageBox::information(this, "成功", msg);
    }
}
