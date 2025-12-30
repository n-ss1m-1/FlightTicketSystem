#include "FlightsPage.h"
#include "ui_FlightsPage.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include "Common/Models.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>
#include "PassengerPickDialog.h"
#include <QShowEvent>
#include "../OrdersPage/OrderDetailDialog.h"
#include <QTimer>

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

    ui->deMinDate->setDate(QDate::currentDate());
    ui->deMaxDate->setDate(QDate::currentDate());

    requestCityList();
}

FlightsPage::~FlightsPage() { delete ui; }

void FlightsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 只刷新城市列表，不触发城市回来后自动查航班
    m_waitingCityListForSearch = false;

    requestCityList();
}


void FlightsPage::requestCityList()
{
    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_CITY_LIST);
    root.insert(Protocol::KEY_DATA, QJsonObject());
    NetworkManager::instance()->sendJson(root);
}

void FlightsPage::sendFlightSearch(const QString& from,
                                   const QString& to,
                                   const QDate& minDate,
                                   const QDate& maxDate,
                                   const QTime& minTime,
                                   const QTime& maxTime)
{
    QJsonObject data;

    // 所有条件允许不指定：不指定就不insert
    if (!from.isEmpty()) data.insert("fromCity", from);
    if (!to.isEmpty())   data.insert("toCity", to);

    QJsonObject dateObj;

    if (minDate.isValid()) dateObj.insert("minDepartDate", minDate.toString("yyyy-MM-dd"));
    if (maxDate.isValid()) dateObj.insert("maxDepartDate", maxDate.toString("yyyy-MM-dd"));

    if (minTime.isValid()) dateObj.insert("minDepartTime", minTime.toString("HH:mm"));
    if (maxTime.isValid()) dateObj.insert("maxDepartTime", maxTime.toString("HH:mm"));

    if (!dateObj.isEmpty())
        data.insert("date", dateObj);

    QJsonObject root;
    root.insert(Protocol::KEY_TYPE, Protocol::TYPE_FLIGHT_SEARCH);
    root.insert(Protocol::KEY_DATA, data);

    m_waitingFlightSearch = true;

    NetworkManager::instance()->sendJson(root);
}


void FlightsPage::on_btnSearch_clicked()
{
    if (!NetworkManager::instance()->isLoggedIn()) {
        QMessageBox::warning(this, "提示", "请先登录后再查询航班。");
        return;
    }

    QString from = ui->comboDep->currentText().trimmed();
    QString to   = ui->comboDest->currentText().trimmed();

    if (from == "不限") from.clear();
    if (to   == "不限") to.clear();

    if (!from.isEmpty() && !to.isEmpty() && from == to) {
        QMessageBox::warning(this, "提示", "出发地和目的地不能相同。");
        return;
    }

    QDate minDate, maxDate;
    QTime minTime, maxTime;

    if (ui->cbUseDateRange->isChecked()) {
        minDate = ui->deMinDate->date();
        maxDate = ui->deMaxDate->date();

        if (minDate.isValid() && maxDate.isValid() && minDate > maxDate) {
            QMessageBox::warning(this, "提示", "最早日期不能晚于最晚日期。");
            return;
        }
    }

    if (ui->cbUseTimeRange->isChecked()) {
        minTime = ui->teMinTime->time();
        maxTime = ui->teMaxTime->time();

        if (minTime.isValid() && maxTime.isValid() && minTime > maxTime) {
            QMessageBox::warning(this, "提示", "最早时间不能晚于最晚时间（不支持跨天范围）。");
            return;
        }
    }

    m_waitingCityListForSearch = true;
    m_pendingFromCity = from;
    m_pendingToCity = to;
    m_pendingMinDate = minDate;
    m_pendingMaxDate = maxDate;
    m_pendingMinTime = minTime;
    m_pendingMaxTime = maxTime;

    requestCityList();
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

                Common::FlightInfo flt = m_flightCache.value(m_pendingBookFlightId);

                PassengerPickDialog dlg(self, {}, flt, this);
                if (dlg.exec() == QDialog::Accepted) {
                    auto chosen = dlg.selectedPassenger();
                    sendCreateOrder(m_pendingBookFlightId, chosen.name, chosen.idCard);
                }
                return;
            }

            return;
        }

        // 航班搜索
        if (m_waitingFlightSearch) {
            m_waitingFlightSearch = false;

            model->removeRows(0, model->rowCount());

            if (msg.contains("暂无")) {
                QMessageBox::information(this, "查询结果", "抱歉，未找到符合条件的航班。");
            } else {
                QMessageBox::warning(this, "航班查询失败", msg);
            }
            return;
        }

        return;
    }

    // 2. 城市列表返回：填充combo
    if (type == Protocol::TYPE_CITY_LIST_RESP) {
        const bool ok = obj.value(Protocol::KEY_SUCCESS).toBool();
        const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

        if (!ok) {
            m_cityListLoaded = false;

            if (m_waitingCityListForSearch) {
                m_waitingCityListForSearch = false;
                QMessageBox::warning(this, "城市列表", msg);

                // 继续查航班（用当前combo的值，或者pending）
                const QString from = ui->comboDep->currentText().trimmed().isEmpty() ? m_pendingFromCity : ui->comboDep->currentText().trimmed();
                const QString to   = ui->comboDest->currentText().trimmed().isEmpty() ? m_pendingToCity : ui->comboDest->currentText().trimmed();
                sendFlightSearch(m_pendingFromCity,
                                 m_pendingToCity,
                                 m_pendingMinDate, m_pendingMaxDate,
                                 m_pendingMinTime, m_pendingMaxTime);
            }
            return;
        }

        QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        QList<QString> fromCities = Common::citiesFromJsonArray(dataObj.value("fromCities").toArray());
        QList<QString> toCities   = Common::citiesFromJsonArray(dataObj.value("toCities").toArray());

        // 记录旧选中，尽量保持用户选择
        const QString oldFrom = ui->comboDep->currentText();
        const QString oldTo   = ui->comboDest->currentText();

        ui->comboDep->blockSignals(true);
        ui->comboDest->blockSignals(true);

        ui->comboDep->clear();
        ui->comboDep->addItem("不限");
        for (const auto &c : fromCities) ui->comboDep->addItem(c);

        ui->comboDest->clear();
        ui->comboDest->addItem("不限");
        for (const auto &c : toCities) ui->comboDest->addItem(c);

        // 恢复旧选择
        int idxFrom = ui->comboDep->findText(oldFrom);
        if (idxFrom >= 0) ui->comboDep->setCurrentIndex(idxFrom);

        int idxTo = ui->comboDest->findText(oldTo);
        if (idxTo >= 0) ui->comboDest->setCurrentIndex(idxTo);

        ui->comboDep->blockSignals(false);
        ui->comboDest->blockSignals(false);

        m_cityListLoaded = true;

        // 如果这是“为了查航班而拉城市”，这里继续发 flight_search
        if (m_waitingCityListForSearch) {
            m_waitingCityListForSearch = false;

            // 尝试把 pending 的城市设回去（如果存在）
            int pFrom = ui->comboDep->findText(m_pendingFromCity);
            if (pFrom >= 0) ui->comboDep->setCurrentIndex(pFrom);

            int pTo = ui->comboDest->findText(m_pendingToCity);
            if (pTo >= 0) ui->comboDest->setCurrentIndex(pTo);

            sendFlightSearch(m_pendingFromCity,
                             m_pendingToCity,
                             m_pendingMinDate, m_pendingMaxDate,
                             m_pendingMinTime, m_pendingMaxTime);
        }
        return;
    }

    // 3. 处理查询结果
    if (type == Protocol::TYPE_FLIGHT_SEARCH_RESP) {
        m_waitingFlightSearch = false;

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
            m_flightCache[f.id] = f;

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
        const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

        const QJsonObject dataObj  = obj.value(Protocol::KEY_DATA).toObject();
        const QJsonObject orderObj = dataObj.value("order").toObject();
        Common::OrderInfo ord = Common::orderFromJson(orderObj);

        QMessageBox::information(this, "下单成功", msg);

        auto reply = QMessageBox::question(this, "立即支付？", "是否现在支付该订单？\n若不支付，稍后可在订单详情页面进行支付。",
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (reply != QMessageBox::Yes) return;

        Common::FlightInfo flt = m_flightCache.value(ord.flightId);

        // 退出当前 onJsonReceived 调用栈后再打开支付窗，避免 processBuffer 重入
        QTimer::singleShot(0, this, [this, ord, flt]() mutable {
            auto *dlg = new OrderDetailDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->setData(ord, flt, "本用户");

            connect(dlg, &OrderDetailDialog::orderPaid, this, [this, dlg](qint64 orderId){
                emit requestGoOrders(orderId);
                dlg->accept();
            });

            dlg->open();
        });

        return;
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

        qDebug() << "[PickDlg] pendingFlightId=" << m_pendingBookFlightId
                 << "cacheHas=" << m_flightCache.contains(m_pendingBookFlightId)
                 << "cacheSize=" << m_flightCache.size();

        Common::FlightInfo flt = m_flightCache.value(m_pendingBookFlightId);
        PassengerPickDialog dlg(self, others, flt, this);
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


void FlightsPage::on_cbUseDateRange_clicked()
{
    ui->deMinDate->setEnabled(ui->cbUseDateRange->isEnabled());
    ui->deMaxDate->setEnabled(ui->cbUseDateRange->isEnabled());
}


void FlightsPage::on_cbUseTimeRange_clicked()
{
    ui->teMinTime->setEnabled(ui->cbUseTimeRange->isEnabled());
    ui->teMaxTime->setEnabled(ui->cbUseTimeRange->isEnabled());
}

