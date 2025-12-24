#include "ServerWindow.h"
#include "ui_ServerWindow.h"
#include "DBManager.h"
#include "OnlineUserManager.h"
#include <QDateTime>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include "AddFlightDialog.h"
#include "AddOrderDialog.h"
#include "AddUserDialog.h"

// 全局指针，方便把日志传给窗口
static ServerWindow* g_window = nullptr;

// 1. 自定义消息处理函数 (拦截 qDebug)
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    QString txt;
    switch (type) {
    case QtDebugMsg:    txt = QString("[Debug] %1").arg(msg); break;
    case QtInfoMsg:     txt = QString("[Info]  %1").arg(msg); break;
    case QtWarningMsg:  txt = QString("[Warn]  %1").arg(msg); break;
    case QtCriticalMsg: txt = QString("[Error] %1").arg(msg); break;
    case QtFatalMsg:    txt = QString("[Fatal] %1").arg(msg); break;
    }
    // 打印到控制台
    fprintf(stdout, "%s\n", txt.toLocal8Bit().constData());
    fflush(stdout);
    // 发送到窗口
    if (g_window) g_window->appendLog(txt);
}

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ServerWindow)
    , m_server(new FlightServer(this))
{
    ui->setupUi(this);
    g_window = this;

    // 安装日志拦截器
    qInstallMessageHandler(myMessageOutput);
    connect(this, &ServerWindow::logSignal, this, &ServerWindow::onLogReceived);

    initTables();

    // 2. 连接数据库
    QString errMsg;
    // TODO: 注意！如果连不上数据库，请把下面的 " 密码" 改成你自己的密码！

    bool connected = DBManager::instance().connect("127.0.0.1", 3306, "root"," 你的密码 ", "flight_ticket", &errMsg);
    if(connected) qInfo() << "GUI初始化：数据库连接成功";
    else qCritical() << "GUI初始化：数据库连接失败 " << errMsg;
}

ServerWindow::~ServerWindow()
{
    qInstallMessageHandler(nullptr);
    delete ui;
}

void ServerWindow::appendLog(const QString &msg) {
    if (g_window) emit g_window->logSignal(msg);
}

void ServerWindow::onLogReceived(const QString &msg) {
    ui->logEditor->append(msg);
}

void ServerWindow::initTables() {
    // 设置表头
    if(ui->tableOnline) {
        QStringList header; header << "用户名" << "姓名" << "电话" << "状态";
        ui->tableOnline->setColumnCount(header.size());
        ui->tableOnline->setHorizontalHeaderLabels(header);
    }
    if(ui->tableFlights) {
        QStringList header; header << "ID" << "航班号" << "出发" << "到达" << "时间" << "余票";
        ui->tableFlights->setColumnCount(header.size());
        ui->tableFlights->setHorizontalHeaderLabels(header);
    }
}

void ServerWindow::on_btnStart_clicked() {
    if (m_server->start(12345)) {
        ui->btnStart->setEnabled(false);
        ui->btnStop->setEnabled(true);
        QMessageBox::information(this, "成功", "服务器已启动 (端口12345)");
    } else {
        QMessageBox::critical(this, "失败", "服务器启动失败，端口可能被占用");
    }
}

void ServerWindow::on_btnStop_clicked() {
    // 暂时没有停止逻辑，只是模拟
    qInfo() << "停止服务信号已发送...";
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);
}
void ServerWindow::on_btnRefresh_clicked()
{
    refreshOnlineUsers();
    refreshFlights();
    refreshAllUsers();
    refreshOrders();
    qInfo() << "数据已手动刷新";
}
void ServerWindow::refreshOnlineUsers() {
    if(!ui->tableOnline) return;
    QList<Common::UserInfo> users = OnlineUserManager::instance().getAllUsers();
    ui->tableOnline->setRowCount(0);
    for(const auto& u : users) {
        int row = ui->tableOnline->rowCount();
        ui->tableOnline->insertRow(row);
        ui->tableOnline->setItem(row, 0, new QTableWidgetItem(u.username));
        ui->tableOnline->setItem(row, 1, new QTableWidgetItem(u.realName));
        ui->tableOnline->setItem(row, 2, new QTableWidgetItem(u.phone));
        ui->tableOnline->setItem(row, 3, new QTableWidgetItem("在线"));
    }
}

void ServerWindow::refreshFlights() {
    if(!ui->tableFlights) return;
    // 直接用 SQL 查询，绕过 DBManager 接口缺失的问题
    QString sql = "SELECT * FROM flight ORDER BY id DESC";
    QSqlQuery query = DBManager::instance().Query(sql);

    ui->tableFlights->setRowCount(0);
    while (query.next()) {
        int row = ui->tableFlights->rowCount();
        ui->tableFlights->insertRow(row);
        ui->tableFlights->setItem(row, 0, new QTableWidgetItem(query.value("id").toString()));
        ui->tableFlights->setItem(row, 1, new QTableWidgetItem(query.value("flight_no").toString()));
        ui->tableFlights->setItem(row, 2, new QTableWidgetItem(query.value("from_city").toString()));
        ui->tableFlights->setItem(row, 3, new QTableWidgetItem(query.value("to_city").toString()));
        ui->tableFlights->setItem(row, 4, new QTableWidgetItem(query.value("depart_time").toDateTime().toString("MM-dd HH:mm")));
        ui->tableFlights->setItem(row, 5, new QTableWidgetItem(query.value("seat_left").toString()));
    }
}
// ServerWindow.cpp

// ---------------------------------------------------------
// 新功能 1：刷新所有注册用户 (不仅仅是在线的)
// ---------------------------------------------------------
void ServerWindow::refreshAllUsers()
{
    if(!ui->tableAllUsers) return;

    // 查库：查所有用户
    QString sql = "SELECT * FROM user";
    QSqlQuery query = DBManager::instance().Query(sql);

    // 设置表头
    QStringList header;
    header << "ID" << "用户名" << "真实姓名" << "电话" << "身份证";
    ui->tableAllUsers->setColumnCount(header.size());
    ui->tableAllUsers->setHorizontalHeaderLabels(header);

    ui->tableAllUsers->setRowCount(0);
    while (query.next()) {
        int row = ui->tableAllUsers->rowCount();
        ui->tableAllUsers->insertRow(row);
        ui->tableAllUsers->setItem(row, 0, new QTableWidgetItem(query.value("id").toString()));
        ui->tableAllUsers->setItem(row, 1, new QTableWidgetItem(query.value("username").toString()));
        ui->tableAllUsers->setItem(row, 2, new QTableWidgetItem(query.value("real_name").toString()));
        ui->tableAllUsers->setItem(row, 3, new QTableWidgetItem(query.value("phone").toString()));
        ui->tableAllUsers->setItem(row, 4, new QTableWidgetItem(query.value("id_card").toString()));
    }
}

// ---------------------------------------------------------
// 新功能 2：删除航班
// ---------------------------------------------------------
void ServerWindow::on_btnDeleteFlight_clicked()
{
    // 1. 获取当前选中的行
    int row = ui->tableFlights->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一行航班！");
        return;
    }

    // 2. 获取航班 ID (假设 ID 在第0列)
    QString flightId = ui->tableFlights->item(row, 0)->text();
    QString flightNo = ui->tableFlights->item(row, 1)->text();

    // 3. 弹窗确认
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  "确定要删除航班 " + flightNo + " 吗？",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) return;

    // 4. 执行 SQL 删除
    QString sql = "DELETE FROM flight WHERE id = ?";
    QList<QVariant> params;
    params << flightId;

    QString err;
    int ret = DBManager::instance().update(sql, params, &err);

    if (ret > 0) {
        QMessageBox::information(this, "成功", "删除成功！");
        refreshFlights(); // 刷新表格
    } else {
        QMessageBox::critical(this, "失败", "删除失败: " + err);
    }
}
void ServerWindow::on_btnShowAddDialog_clicked()
{
    // 1. 创建弹窗对象
    AddFlightDialog dlg(this);

    // 2. 显示弹窗，并等待结果
    // dlg.exec() 会阻塞在这里，直到用户点确认(accept)或取消(reject)
    if (dlg.exec() == QDialog::Accepted) {
        // 3. 如果用户添加成功了，我们就刷新表格
        refreshFlights();
    }
}
// ---------------------------------------------------------
// 新功能 4：订单管理 (刷新列表)
// ---------------------------------------------------------
void ServerWindow::refreshOrders()
{
    if(!ui->tableOrders) return;

    // 这是一个多表联合查询 (Join)，为了同时显示 用户名 和 航班号
    // 如果你觉得太复杂，也可以直接 SELECT * FROM orders
    QString sql = "SELECT o.id, u.username, f.flight_no, o.passenger_name, o.price_cents, o.status "
                  "FROM orders o "
                  "LEFT JOIN user u ON o.user_id = u.id "
                  "LEFT JOIN flight f ON o.flight_id = f.id "
                  "ORDER BY o.id DESC";

    QSqlQuery query = DBManager::instance().Query(sql);

    // 设置表头
    QStringList header;
    header << "订单ID" << "下单用户" << "航班号" << "乘客姓名" << "价格" << "状态";
    ui->tableOrders->setColumnCount(header.size());
    ui->tableOrders->setHorizontalHeaderLabels(header);

    ui->tableOrders->setRowCount(0);
    while (query.next()) {
        int row = ui->tableOrders->rowCount();
        ui->tableOrders->insertRow(row);

        ui->tableOrders->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        ui->tableOrders->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        ui->tableOrders->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        ui->tableOrders->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        ui->tableOrders->setItem(row, 4, new QTableWidgetItem(query.value(4).toString()));

        // 状态转文字
        int status = query.value(5).toInt();
        QString statusStr = (status == 0) ? "已预订" : (status == 1) ? "已支付" : "已取消";
        ui->tableOrders->setItem(row, 5, new QTableWidgetItem(statusStr));
    }
}

// ---------------------------------------------------------
// 新功能 5：管理员强制取消订单
// ---------------------------------------------------------
void ServerWindow::on_btnCancelOrder_clicked()
{
    // 1. 获取选中行
    int row = ui->tableOrders->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一个订单！");
        return;
    }

    // 2. 获取订单ID
    QString orderId = ui->tableOrders->item(row, 0)->text();
    QString passenger = ui->tableOrders->item(row, 3)->text();

    // 3. 确认
    if (QMessageBox::question(this, "确认", "确定要强制取消 " + passenger + " 的订单吗？")
        != QMessageBox::Yes) {
        return;
    }

    // 4. 执行 SQL (直接改成已取消状态 2)
    // 注意：这里最好也把座位还回去，简单起见我们先只改状态
    QString sql = "UPDATE orders SET status = 2 WHERE id = ?";
    QList<QVariant> params;
    params << orderId;

    QString err;
    if (DBManager::instance().update(sql, params, &err) > 0) {
        QMessageBox::information(this, "成功", "订单已取消");
        refreshOrders(); // 刷新
    } else {
        QMessageBox::critical(this, "失败", "操作失败: " + err);
    }
}
// ---------------------------------------------------------
// 新功能 6：显示补录订单弹窗
// ---------------------------------------------------------
void ServerWindow::on_btnShowAddOrderDialog_clicked()
{
    AddOrderDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // 如果添加成功，刷新列表
        refreshOrders();
        // 顺便刷新一下航班列表（因为座位数变了）
        refreshFlights();
    }
}
// ---------------------------------------------------------
// 新功能 7：删除用户
// ---------------------------------------------------------
void ServerWindow::on_btnDeleteUser_clicked()
{
    // 1. 获取选中行
    int row = ui->tableAllUsers->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一个用户！");
        return;
    }

    // 2. 获取用户ID (假设ID在第0列) 和 用户名 (第1列)
    QString userId = ui->tableAllUsers->item(row, 0)->text();
    QString username = ui->tableAllUsers->item(row, 1)->text();

    // 3. 弹窗确认
    // 注意：根据数据库设置，删除用户可能会把他的订单 user_id 设为 NULL，或者连带删除
    if (QMessageBox::question(this, "危险操作",
                              "确定要注销用户 [" + username + "] 吗？\n该操作无法撤销！")
        != QMessageBox::Yes) {
        return;
    }

    // 4. 执行删除
    QString sql = "DELETE FROM user WHERE id = ?";
    QList<QVariant> params;
    params << userId;

    QString err;
    if (DBManager::instance().update(sql, params, &err) > 0) {
        QMessageBox::information(this, "成功", "用户已删除");
        refreshAllUsers(); // 刷新列表
        refreshOnlineUsers(); // 万一他正好在线，也刷新一下在线列表
    } else {
        QMessageBox::critical(this, "失败", "删除失败: " + err);
    }
}
// ---------------------------------------------------------
// 新功能 8：显示添加用户弹窗
// ---------------------------------------------------------
void ServerWindow::on_btnShowAddUserDialog_clicked()
{
    AddUserDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshAllUsers(); // 成功后刷新表格
    }
}
