#include "PassengerPickDialog.h"
#include "ui_PassengerPickDialog.h"
#include <QMessageBox>

PassengerPickDialog::PassengerPickDialog(const Common::PassengerInfo& self,
                                         const QList<Common::PassengerInfo>& others,
                                         const Common::FlightInfo& flight,
                                         QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PassengerPickDialog)
    , m_flight(flight)
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
