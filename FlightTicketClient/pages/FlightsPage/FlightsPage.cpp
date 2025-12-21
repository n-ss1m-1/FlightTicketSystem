#include "FlightsPage.h"
#include "ui_FlightsPage.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QStandardItemModel>

FlightsPage::FlightsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FlightsPage)
{
    ui->setupUi(this);
    model = new QSqlQueryModel(this);

    // 日期默认为今天
    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setCalendarPopup(true);

    // 初始化下拉框数据
    QStringList cities;
    cities << "北京" << "上海" << "广州" << "深圳" << "成都" << "杭州";
    ui->comboDep->addItems(cities);
    ui->comboDest->addItems(cities);

    // 设置模拟表头
    QStandardItemModel *testModel = new QStandardItemModel(this);
    testModel->setHorizontalHeaderLabels({"ID", "航班号", "出发地", "目的地", "起飞时间", "票价"});

    // 假数据作为界面预览
    QList<QStandardItem*> row1;
    row1 << new QStandardItem("1") << new QStandardItem("CA1234") << new QStandardItem("北京")
         << new QStandardItem("上海") << new QStandardItem("10:00") << new QStandardItem("￥850");
    testModel->appendRow(row1);

    ui->tableView->setModel(testModel);

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

FlightsPage::~FlightsPage()
{
    delete ui;
}

void FlightsPage::on_btnSearch_clicked()
{
    QString dep = ui->comboDep->currentText();
    QString dest = ui->comboDest->currentText();
    QString date = ui->dateEdit->date().toString("yyyy-MM-dd");

    QString sql = QString("SELECT id, flight_no AS '航班号', departure AS '出发地', "
                          "destination AS '目的地', flight_date AS '日期', price AS '价格' "
                          "FROM flights "
                          "WHERE departure='%1' AND destination='%2' AND flight_date='%3'")
                      .arg(dep).arg(dest).arg(date);

    model->setQuery(sql);

    if (model->lastError().isValid()) {
        QMessageBox::critical(this, "错误", "查询失败: " + model->lastError().text());
    } else {
        ui->tableView->setModel(model);
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setColumnHidden(0, true);
    }
}

void FlightsPage::on_btnBook_clicked()
{
    // 1. 获取选中的行
    int curRow = ui->tableView->currentIndex().row();
    if (curRow < 0) {
        QMessageBox::warning(this, "提示", "请先在列表中点击选择一个航班！");
        return;
    }

    // 2. 取出航班信息 (假设ID在第0列，航班号在第1列)
    QString flightId = model->data(model->index(curRow, 0)).toString();
    QString flightNo = model->data(model->index(curRow, 1)).toString();

    // 3. 确认框
    auto reply = QMessageBox::question(this, "订票确认", "确定要预订航班 " + flightNo + " 吗？");
    if (reply != QMessageBox::Yes) return;

    // 4. 执行订票操作（插入订单记录）
    QSqlQuery query;
    query.prepare("INSERT INTO orders (flight_id, user_id, order_time, status) "
                  "VALUES (:fid, :uid, NOW(), '已支付')");
    query.bindValue(":fid", flightId);
    query.bindValue(":uid", 1); // 默认当前用户ID为1

    if (query.exec()) {
        QMessageBox::information(this, "成功", "订票成功！订单已生成。");
    } else {
        QMessageBox::critical(this, "订票失败", "数据库错误: " + query.lastError().text());
    }
}
