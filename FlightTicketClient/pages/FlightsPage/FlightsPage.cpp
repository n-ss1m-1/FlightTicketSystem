#include "FlightsPage.h"
#include "ui_FlightsPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>
#include "PassengerPickDialog.h"

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
    qDebug() << "username=" << NetworkManager::instance()->m_username
             << "realName=" << NetworkManager::instance()->m_userInfo.realName
             << "idCard=" << NetworkManager::instance()->m_userInfo.idCard;


    if (!NetworkManager::instance()->isLoggedIn()) {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }

    int row = ui->tableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择一个航班！");
        return;
    }

    qint64 flightId = model->data(model->index(row, 0)).toLongLong();

    m_pendingBookFlightId = flightId;
    m_waitingPassengerPick = true;

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_PASSENGER_GET);
    root.insert(Protocol::KEY_DATA, QJsonObject());

    NetworkManager::instance()->sendJson(root);
}


void FlightsPage::onJsonReceived(const QJsonObject &obj)
{
    QString type = obj.value(Protocol::KEY_TYPE).toString();

    // 1. 恢复错误处理逻辑
    if (type == Protocol::TYPE_ERROR) {
        QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

        // 如果这是订票流程里passenger_get的返回，仍然允许选择本用户
        if (m_waitingPassengerPick) {
            m_waitingPassengerPick = false;

            if (msg.contains("暂无常用乘机人")) {
                Common::PassengerInfo self;
                self.name  = NetworkManager::instance()->m_userInfo.realName;
                self.idCard= NetworkManager::instance()->m_userInfo.idCard;
                self.user_id = NetworkManager::instance()->m_userInfo.id;

                PassengerPickDialog dlg(self, {}, this);
                if (dlg.exec() == QDialog::Accepted) {
                    auto chosen = dlg.selectedPassenger();
                    sendCreateOrder(m_pendingBookFlightId, chosen.name, chosen.idCard);
                }
                return;
            }

            // 其它错误：提示
            QMessageBox::warning(this, "获取乘机人失败", msg);
            return;
        }

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

    // passenger_get 返回：弹出 PassengerPickDialog
    else if (type == Protocol::TYPE_PASSENGER_GET_RESP) {
        qDebug() << "[Book] waiting=" << m_waitingPassengerPick;

        if (!m_waitingPassengerPick) return;
        m_waitingPassengerPick = false;

        QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        QJsonArray arr = dataObj.value("passengers").toArray();
        QList<Common::PassengerInfo> others = Common::passengersFromJsonArray(arr);

        // 从登录用户信息拿真实姓名/身份证
        Common::PassengerInfo self;
        self.name = NetworkManager::instance()->m_userInfo.realName;
        self.idCard = NetworkManager::instance()->m_userInfo.idCard;

        PassengerPickDialog dlg(self, others, this);
        if (dlg.exec() == QDialog::Accepted) {
            auto chosen = dlg.selectedPassenger();
            sendCreateOrder(m_pendingBookFlightId, chosen.name, chosen.idCard);
        }
        return;
    }

}

void FlightsPage::sendCreateOrder(qint64 flightId, const QString& name, const QString& idCard)
{
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

