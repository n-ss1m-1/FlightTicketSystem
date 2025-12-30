#include "PassengerPickDialog.h"
#include "ui_PassengerPickDialog.h"
#include <QMessageBox>
#include "../ProfilePage/AddPassengerDialog.h"
#include "NetworkManager.h"
#include "Common/Protocol.h"

PassengerPickDialog::PassengerPickDialog(const Common::PassengerInfo& self,
                                         const QList<Common::PassengerInfo>& others,
                                         const Common::FlightInfo& flight,
                                         QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PassengerPickDialog)
    , m_flight(flight)
    , m_self(self)
{
    ui->setupUi(this);
    setWindowTitle("选择乘机人");
    setModal(true);

    // 候选列表：第一个固定本用户
    m_all.clear();
    m_all << self;
    for (const auto& p : others) m_all << p;

    initTable();
    loadData();

    updateFlightInfoUi();

    // 默认选中第一行（本用户）
    if (m_model->rowCount() > 0) {
        ui->tablePassengers->selectRow(0);
        m_selectedRow = 0;
    }

    connect(ui->tablePassengers->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex& current, const QModelIndex&) {
                m_selectedRow = current.row();
            });
}

PassengerPickDialog::~PassengerPickDialog()
{
    delete ui;
}

QString PassengerPickDialog::maskIdCard(const QString& id)
{
    return id.left(6) + "********" + id.right(4);
}

void PassengerPickDialog::initTable()
{
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"来源", "姓名", "身份证号"});

    ui->tablePassengers->setModel(m_model);
    ui->tablePassengers->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tablePassengers->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tablePassengers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tablePassengers->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void PassengerPickDialog::loadData()
{
    m_model->removeRows(0, m_model->rowCount());

    for (int i = 0; i < m_all.size(); ++i) {
        const auto& p = m_all[i];
        const QString source = (i == 0) ? "本用户" : "常用乘机人";

        QList<QStandardItem*> row;
        row << new QStandardItem(source);
        row << new QStandardItem(p.name);
        row << new QStandardItem(maskIdCard(p.idCard));

        m_model->appendRow(row);
    }
}

Common::PassengerInfo PassengerPickDialog::selectedPassenger() const
{
    if (m_selectedRow < 0 || m_selectedRow >= m_all.size()) return Common::PassengerInfo{};
    return m_all[m_selectedRow];
}

void PassengerPickDialog::on_btnOk_clicked()
{
    if (m_selectedRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择一名乘机人。");
        return;
    }
    accept();
}

void PassengerPickDialog::on_btnCancel_clicked()
{
    reject();
}

void PassengerPickDialog::updateFlightInfoUi()
{
    ui->lblFlightNo->setText(m_flight.flightNo.isEmpty() ? "--" : m_flight.flightNo);
    ui->lblRoute->setText(QString("%1 → %2")
                              .arg(m_flight.fromCity.isEmpty() ? "--" : m_flight.fromCity)
                              .arg(m_flight.toCity.isEmpty() ? "--" : m_flight.toCity));

    auto dtToText = [](const QDateTime& dt){
        return dt.isValid() ? dt.toString("yyyy-MM-dd HH:mm") : QString("--");
    };

    ui->lblDepartTime->setText(dtToText(m_flight.departTime));
    ui->lblArriveTime->setText(dtToText(m_flight.arriveTime));

    ui->lblPrice->setText(QString("￥%1").arg(QString::number(m_flight.priceCents / 100.0, 'f', 2)));
}

void PassengerPickDialog::on_btnAddPassenger_clicked()
{
    AddPassengerDialog dlg(this);

    // 提交后发请求
    connect(&dlg, &AddPassengerDialog::passengerSubmitted, this,
        [this](const QString& name, const QString& idCard)
        {
            auto nm = NetworkManager::instance();
            if (!nm->isConnected() || !nm->isLoggedIn()) {
                QMessageBox::warning(this, "提示", "请先登录再添加乘机人。");
                return;
            }

            const QString n  = name.trimmed();
            const QString id = idCard.trimmed().toUpper();

            for (int i = 1; i < m_all.size(); ++i) {
                if (m_all[i].idCard.compare(id, Qt::CaseInsensitive) == 0) {
                    QMessageBox::information(this, "提示", "该乘机人已存在。");
                    return;
                }
            }

            QJsonObject data;
            data.insert("passenger_name", n);
            data.insert("passenger_id_card", id);

            QJsonObject req;
            req.insert(Protocol::KEY_TYPE, Protocol::TYPE_PASSENGER_ADD);
            req.insert(Protocol::KEY_DATA, data);

            if (m_addConn) QObject::disconnect(m_addConn);

            m_addConn = connect(nm, &NetworkManager::jsonReceived, this,
                [this, n, id](const QJsonObject &obj)
                {
                    const QString type = obj.value(Protocol::KEY_TYPE).toString();

                    if (type == Protocol::TYPE_ERROR) {
                        QMessageBox::critical(this, "错误", obj.value(Protocol::KEY_MESSAGE).toString());
                        QObject::disconnect(m_addConn);
                        m_addConn = {};
                        return;
                    }

                    if (type == Protocol::TYPE_PASSENGER_ADD_RESP) {
                        const bool ok = obj.value(Protocol::KEY_SUCCESS).toBool();
                        const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

                        if (!ok) QMessageBox::critical(this, "添加失败", msg);
                        else     QMessageBox::information(this, "添加成功", msg);

                        QObject::disconnect(m_addConn);
                        m_addConn = {};

                        if (ok) {
                            // 添加成功后从服务器重新拉取列表（拿到新 passenger.id）
                            requestPassengers();
                        }
                        return;
                    }
                });

            nm->sendJson(req);
        });

    dlg.exec();
}

void PassengerPickDialog::requestPassengers()
{
    auto nm = NetworkManager::instance();
    if (!nm->isConnected() || !nm->isLoggedIn()) return;

    QJsonObject req;
    req.insert(Protocol::KEY_TYPE, Protocol::TYPE_PASSENGER_GET);
    req.insert(Protocol::KEY_DATA, QJsonObject{});

    if (m_getConn) QObject::disconnect(m_getConn);

    m_getConn = connect(nm, &NetworkManager::jsonReceived, this,
        [this](const QJsonObject &obj)
        {
            QJsonDocument doc(obj);
            qDebug() << "==== [服务器返回的原始数据] ====";
            qDebug() << doc.toJson(QJsonDocument::Indented);
            qDebug() << "================================";

            const QString type = obj.value(Protocol::KEY_TYPE).toString();

            if (type == Protocol::TYPE_ERROR) {
                const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

                if (msg.contains("暂无")) {
                    reloadAll({});
                } else {
                    QMessageBox::critical(this, "错误", msg);
                }

                QObject::disconnect(m_getConn);
                m_getConn = {};
                return;
            }

            if (type == Protocol::TYPE_PASSENGER_GET_RESP) {
                const QJsonObject dataObj = obj.value(Protocol::KEY_DATA).toObject();
                const QJsonArray arr = dataObj.value("passengers").toArray();

                QList<Common::PassengerInfo> others;
                for (auto v : arr) {
                    others << Common::passengerFromJson(v.toObject());
                }

                reloadAll(others);

                QObject::disconnect(m_getConn);
                m_getConn = {};
                return;
            }
        });

    nm->sendJson(req);
}

void PassengerPickDialog::reloadAll(const QList<Common::PassengerInfo>& others)
{
    m_all.clear();
    m_all << m_self;
    for (const auto& p : others) m_all << p;

    loadData();

    if (m_model->rowCount() > 0) {
        ui->tablePassengers->selectRow(0);
        m_selectedRow = 0;
    }
}


