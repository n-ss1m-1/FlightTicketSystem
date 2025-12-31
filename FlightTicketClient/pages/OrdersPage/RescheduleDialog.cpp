#include "RescheduleDialog.h"
#include "ui_RescheduleDialog.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QPointer>

static QString centsToYuanText2(qint32 cents)
{
    return QString::number(cents / 100.0, 'f', 2);
}

RescheduleDialog::RescheduleDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RescheduleDialog)
{
    ui->setupUi(this);

    setWindowTitle("改签航班选择");
    setModal(true);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"航班ID","航班号","出发地","目的地","起飞时间","到达时间","票价","余票"});
    ui->tableFlights->setModel(m_model);
    ui->tableFlights->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableFlights->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableFlights->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableFlights->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    connect(ui->btnClose, &QPushButton::clicked, this, &RescheduleDialog::reject);
}

RescheduleDialog::~RescheduleDialog()
{
    delete ui;
}

void RescheduleDialog::setData(const Common::OrderInfo &oriOrder,
                               const Common::FlightInfo &oriFlight)
{
    m_oriOrder  = oriOrder;
    m_oriFlight = oriFlight;

    ui->lblRoute->setText(QString("%1 → %2")
        .arg(oriFlight.fromCity.isEmpty() ? "--" : oriFlight.fromCity)
        .arg(oriFlight.toCity.isEmpty() ? "--" : oriFlight.toCity));

    if (oriFlight.departTime.isValid()){
        ui->deMinDate->setDate(oriFlight.departTime.date());
        ui->deMaxDate->setDate(oriFlight.departTime.date());
    }
}

void RescheduleDialog::on_btnSearch_clicked()
{
    sendFlightSearch();
}

void RescheduleDialog::sendFlightSearch()
{
    if (m_oriFlight.fromCity.isEmpty() || m_oriFlight.toCity.isEmpty()) {
        QMessageBox::warning(this, "提示", "原订单航班出发地/目的地为空，无法改签查询。");
        return;
    }
    if (!NetworkManager::instance()->isLoggedIn()) {
        QMessageBox::warning(this, "提示", "未登录，无法查询航班。");
        return;
    }

    // 避免叠加连接
    if (m_searchConn) {
        disconnect(m_searchConn);
        m_searchConn = {};
    }

    m_waitingSearch = true;

    QJsonObject data;
    data["fromCity"] = m_oriFlight.fromCity;
    data["toCity"]   = m_oriFlight.toCity;

    QJsonObject dateObj;
    dateObj["minDepartDate"] = ui->deMinDate->date().toString("yyyy-MM-dd");
    dateObj["maxDepartDate"] = ui->deMaxDate->date().toString("yyyy-MM-dd");
    data["date"] = dateObj;

    QJsonObject req;
    req[Protocol::KEY_TYPE] = Protocol::TYPE_FLIGHT_SEARCH;
    req[Protocol::KEY_DATA] = data;

    auto nm = NetworkManager::instance();
    QPointer<RescheduleDialog> self(this);

    m_searchConn = connect(nm, &NetworkManager::jsonReceived, this, [=](const QJsonObject &obj) mutable {
        if (!self) {
            if (m_searchConn) { disconnect(m_searchConn); m_searchConn = {}; }
            return;
        }

        const QString type = obj.value(Protocol::KEY_TYPE).toString();

        // 只处理本次查询期间的回包
        if (!m_waitingSearch) return;

        if (type == Protocol::TYPE_ERROR) {
            m_waitingSearch = false;
            if (m_searchConn) { disconnect(m_searchConn); m_searchConn = {}; }

            QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

            if(!msg.contains("暂无")){
                QMessageBox::warning(self, "航班查询失败", msg);
            }
            else{
                QMessageBox::information(self, "查询结果", "抱歉，未找到符合条件的航班。");
            }
            return;
        }

        if (type != Protocol::TYPE_FLIGHT_SEARCH_RESP) return;

        m_waitingSearch = false;
        if (m_searchConn) { disconnect(m_searchConn); m_searchConn = {}; }

        const QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        const QJsonArray flightsArr = dataObj.value("flights").toArray();

        self->m_flights = Common::flightsFromJsonArray(flightsArr);

        // 过滤不允许同航班
        self->m_showFlights.clear();
        self->m_model->removeRows(0, self->m_model->rowCount());

        for (const auto &f : self->m_flights) {

            if (f.fromCity != self->m_oriFlight.fromCity || f.toCity != self->m_oriFlight.toCity)
                continue;

            const bool sameNo   = (!self->m_oriFlight.flightNo.isEmpty() && f.flightNo == self->m_oriFlight.flightNo);
            const bool sameTime = (self->m_oriFlight.departTime.isValid() && f.departTime == self->m_oriFlight.departTime);

            if (sameNo && sameTime) {
                continue;
            }

            self->m_showFlights.append(f);

            QList<QStandardItem*> row;
            row << new QStandardItem(QString::number(f.id));
            row << new QStandardItem(f.flightNo);
            row << new QStandardItem(f.fromCity);
            row << new QStandardItem(f.toCity);
            row << new QStandardItem(f.departTime.toString("yyyy-MM-dd HH:mm"));
            row << new QStandardItem(f.arriveTime.toString("yyyy-MM-dd HH:mm"));
            row << new QStandardItem(centsToYuanText2(f.priceCents));
            row << new QStandardItem(QString::number(f.seatLeft));
            self->m_model->appendRow(row);
        }

        if(m_showFlights.empty()){
            QMessageBox::warning(self, "查询失败", "没有符合条件的航班");
        }
    });

    nm->sendJson(req);
}


void RescheduleDialog::on_btnConfirm_clicked()
{
    const QModelIndex idx = ui->tableFlights->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::warning(this, "提示", "请选择一个航班");
        return;
    }

    const int row = idx.row();
    if (row < 0 || row >= m_flights.size()) {
        QMessageBox::warning(this, "提示", "选中航班无效");
        return;
    }

    const auto selectedFlight = m_flights[row];
    if (selectedFlight.seatLeft <= 0) {
        QMessageBox::warning(this, "提示", "该航班余票不足");
        return;
    }

    sendReschedule(selectedFlight.id);
}

void RescheduleDialog::sendReschedule(qint64 newFlightId)
{
    if (NetworkManager::instance()->m_username.isEmpty()) {
        QMessageBox::warning(this, "提示", "未登录，无法改签。");
        return;
    }

    // 找出选中航班对象（用于成功后回传给订单详情页）
    Common::FlightInfo selectedFlight;
    bool found = false;
    for (const auto &f : m_flights) {
        if (f.id == newFlightId) { selectedFlight = f; found = true; break; }
    }
    if (!found) {
        QMessageBox::warning(this, "提示", "未找到选中航班信息");
        return;
    }

    QJsonObject data;
    data["oriOrder"] = Common::orderToJson(m_oriOrder);
    data["flightId"] = static_cast<qint64>(newFlightId);
    data["passengerName"] = m_oriOrder.passengerName;
    data["passengerIdCard"] = m_oriOrder.passengerIdCard;

    QJsonObject req;
    req[Protocol::KEY_TYPE] = Protocol::TYPE_ORDER_RESCHEDULE;
    req[Protocol::KEY_DATA] = data;

    auto nm = NetworkManager::instance();
    QPointer<RescheduleDialog> self(this);

    QMetaObject::Connection conn;
    conn = connect(nm, &NetworkManager::jsonReceived, this, [=](const QJsonObject &obj) mutable {
        if (!self) { disconnect(conn); return; }

        const QString type = obj.value(Protocol::KEY_TYPE).toString();

        if (type == Protocol::TYPE_ERROR) {
            QMessageBox::critical(self, "改签失败", obj.value(Protocol::KEY_MESSAGE).toString());
            disconnect(conn);
            return;
        }

        if (type != Protocol::TYPE_ORDER_RESCHEDULE_RESP) return;
        disconnect(conn);

        const QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
        const QJsonObject orderObj = dataObj.value("order").toObject();
        const qint32 priceDif = dataObj.value("priceDif").toInt();

        const Common::OrderInfo newOrder = Common::orderFromJson(orderObj);

        // 差价提示
        if (priceDif > 0) {
            QMessageBox::information(self, "改签成功",
                                     QString("改签成功，需要补差价 ¥%1\n新订单号：%2")
                                         .arg(centsToYuanText2(priceDif))
                                         .arg(newOrder.id));
        } else if (priceDif < 0) {
            QMessageBox::information(self, "改签成功",
                                     QString("改签成功，应退款 ¥%1\n新订单号：%2")
                                         .arg(centsToYuanText2(-priceDif))
                                         .arg(newOrder.id));
        } else {
            QMessageBox::information(self, "改签成功",
                                     QString("改签成功，无需补差价\n新订单号：%1").arg(newOrder.id));
        }

        emit self->rescheduled(newOrder, selectedFlight, priceDif);
        self->accept();
    });

    nm->sendJson(req);
}

