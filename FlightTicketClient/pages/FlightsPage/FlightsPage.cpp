#include "FlightsPage.h"
#include "ui_FlightsPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"  // 必须引入 Models.h 以使用 Common 命名空间下的工具
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>

FlightsPage::FlightsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FlightsPage)
{
    ui->setupUi(this);

    model = new QStandardItemModel(this);
    // 重新设置表头，使其更符合实际显示需求：ID, 航班号, 出发地, 目的地, 起飞时间, 到达时间, 票价
    model->setHorizontalHeaderLabels({"ID", "航班号", "出发地", "目的地", "起飞时间", "到达时间", "票价"});
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

void FlightsPage::on_btnSearch_clicked()
{
    QJsonObject data;
    data.insert("fromCity", ui->comboDep->currentText());
    data.insert("toCity", ui->comboDest->currentText());
    // 关键：服务端使用 QDate::fromString(..., Qt::ISODate)，这里发送 yyyy-MM-dd 格式
    data.insert("date", ui->dateEdit->date().toString(Qt::ISODate));

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_FLIGHT_SEARCH);
    root.insert(Protocol::KEY_DATA, data);

    NetworkManager::instance()->sendJson(root);
}

void FlightsPage::on_btnBook_clicked()
{
    int row = ui->tableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择一个航班！");
        return;
    }

    // 从表格获取 ID (第0列)
    qint64 flightId = model->data(model->index(row, 0)).toLongLong();

    bool ok;
    QString name = QInputDialog::getText(this, "乘客信息", "请输入乘客姓名:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QString idCard = QInputDialog::getText(this, "乘客信息", "请输入身份证号:", QLineEdit::Normal, "", &ok);
    if (!ok || idCard.isEmpty()) return;

    QJsonObject data;
    data.insert("userId", 1); // 暂时固定为1
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

    // 如果返回的是错误类型
    if (type == Protocol::TYPE_ERROR) {
        QMessageBox::critical(this, "错误", obj.value(Protocol::KEY_MESSAGE).toString());
        return;
    }

    // 处理查询结果
    if (type == Protocol::TYPE_FLIGHT_SEARCH_RESP) {
        QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        QJsonArray flightsArr = dataObj.value("flights").toArray();

        model->removeRows(0, model->rowCount()); // 清空旧数据

        for (int i = 0; i < flightsArr.size(); ++i) {
            QJsonObject fObj = flightsArr[i].toObject();

            // 使用 models.h 中定义的解析函数，最稳妥
            Common::FlightInfo f = Common::flightFromJson(fObj);

            QList<QStandardItem*> row;
            // 1. ID
            row << new QStandardItem(QString::number(f.id));
            // 2. 航班号
            row << new QStandardItem(f.flightNo);
            // 3. 出发地
            row << new QStandardItem(f.fromCity);
            // 4. 目的地
            row << new QStandardItem(f.toCity);

            // 5. 起飞时间 (格式化显示：yyyy-MM-dd HH:mm)
            row << new QStandardItem(f.departTime.toString("yyyy-MM-dd HH:mm"));

            // 6. 到达时间 (格式化显示：yyyy-MM-dd HH:mm)
            row << new QStandardItem(f.arriveTime.toString("yyyy-MM-dd HH:mm"));

            // 7. 票价 (将分转换为元显示)
            double priceYuan = f.priceCents / 100.0;
            row << new QStandardItem(QString("￥%1").arg(QString::number(priceYuan, 'f', 2)));

            model->appendRow(row);
        }
        ui->tableView->setColumnHidden(0, true); // 隐藏 ID 列
    }
    // 处理订票结果
    else if (type == Protocol::TYPE_ORDER_CREATE_RESP) {
        QString msg = obj.value(Protocol::KEY_MESSAGE).toString();
        QMessageBox::information(this, "成功", msg);
    }
}
