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
#include <QTcpServer>
#include <QTcpSocket> // 必须包含

// 全局指针，方便把日志传给窗口
static ServerWindow* g_window = nullptr;

// 自定义消息处理函数
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
    fprintf(stdout, "%s\n", txt.toLocal8Bit().constData());
    fflush(stdout);
    if (g_window) g_window->appendLog(txt);
}

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ServerWindow)
    , m_server(new FlightServer(this))
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
    g_window = this;

    // 安装日志拦截器
    qInstallMessageHandler(myMessageOutput);
    connect(this, &ServerWindow::logSignal, this, &ServerWindow::onLogReceived);

    // 关键：连接 Server 的新连接信号，维护 m_clientList
    // 使用强转确保能访问 QTcpServer 的信号
    QTcpServer *tcpServer = (QTcpServer*)m_server;

    connect(tcpServer, &QTcpServer::newConnection, this, [=](){
        while(tcpServer->hasPendingConnections()) {
            QTcpSocket *sock = tcpServer->nextPendingConnection();
            m_clientList.append(sock);

            // 客户端断开时从列表移除
            connect(sock, &QTcpSocket::disconnected, this, [=](){
                m_clientList.removeOne(sock);
                sock->deleteLater();
            });
        }
    });

    initTables();
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
    ui->textEditLog->append(msg);
}

// ---------------------------------------------------------
// 初始化表格 (注意航班表有7列)
// ---------------------------------------------------------
void ServerWindow::initTables() {
    // 1. 在线用户表
    if(ui->tableOnline) {
        QStringList header; header << "用户名" << "姓名" << "电话" << "状态";
        ui->tableOnline->setColumnCount(header.size());
        ui->tableOnline->setHorizontalHeaderLabels(header);
    }
    // 2. 航班表 (7列：ID, 航班号, 出发, 到达, 时间, 余票, price)
    if(ui->tableFlights) {
        QStringList header;
        header << "ID" << "航班号" << "出发" << "到达" << "时间" << "余票" << "价格";
        ui->tableFlights->setColumnCount(header.size());
        ui->tableFlights->setHorizontalHeaderLabels(header);
    }
}

// ---------------------------------------------------------
// 启动逻辑
// ---------------------------------------------------------
void ServerWindow::on_btnStart_clicked() {
    QTcpServer *serverBase = (QTcpServer*)m_server;

    if (serverBase->listen(QHostAddress::Any, 12345)) {
        ui->btnStart->setEnabled(false); // 启动变灰
        ui->btnPause->setEnabled(true);  // 暂停变亮

        // 更新状态标签 (确保UI里有这个Label)
        ui->labelStatus->setText("状态：运行中");

        QMessageBox::information(this, "成功", "服务器已启动 (端口12345)");
        on_btnRefresh_clicked();
    } else {
        QMessageBox::critical(this, "失败", "服务器启动失败，端口可能被占用");
    }
}

// ---------------------------------------------------------
// 暂停/停止逻辑 (合并了以前的停止功能)
// ---------------------------------------------------------
void ServerWindow::on_btnPause_clicked()
{
    QTcpServer *serverBase = (QTcpServer*)m_server;

    // 1. 停止监听新连接
    if (serverBase->isListening()) {
        serverBase->close();
        ui->textEditLog->append("[Info] 服务已停止监听。");
    }

    // 2. 通知并断开所有客户端
    for (QTcpSocket *socket : m_clientList) {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->write("CMD:SERVER_PAUSED");
            socket->waitForBytesWritten(1000);
            socket->disconnectFromHost();
        }
    }

    // 3. 界面重置为“可启动”状态
    ui->btnStart->setEnabled(true);  // 允许再次点击启动
    ui->btnPause->setEnabled(false); // 禁用暂停
    ui->labelStatus->setText("状态：已暂停");
}

// 注意：on_btnStop_clicked 已经被彻底删除

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

// ---------------------------------------------------------
// 刷新航班 (修复余票覆盖问题)
// ---------------------------------------------------------
void ServerWindow::refreshFlights() {
    if(!ui->tableFlights) return;

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

        QDateTime depTime = query.value("depart_time").toDateTime();
        ui->tableFlights->setItem(row, 4, new QTableWidgetItem(depTime.toString("yyyy-MM-dd HH:mm")));

        // 第5列：余票
        ui->tableFlights->setItem(row, 5, new QTableWidgetItem(query.value("seat_left").toString()));

        // 第6列：价格
        int priceCents = query.value("price_cents").toInt();
        double priceYuan = priceCents / 100.0;
        ui->tableFlights->setItem(row, 6, new QTableWidgetItem(QString::number(priceYuan, 'f', 2) + " 元"));
    }
}

void ServerWindow::refreshAllUsers()
{
    if(!ui->tableAllUsers) return;

    QString sql = "SELECT * FROM user";
    QSqlQuery query = DBManager::instance().Query(sql);

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

void ServerWindow::on_btnDeleteFlight_clicked()
{
    int row = ui->tableFlights->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一行航班！");
        return;
    }

    QString flightId = ui->tableFlights->item(row, 0)->text();
    QString flightNo = ui->tableFlights->item(row, 1)->text();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  "确定要删除航班 " + flightNo + " 吗？",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) return;

    QString sql = "DELETE FROM flight WHERE id = ?";
    QList<QVariant> params;
    params << flightId;

    QString err;
    int ret = DBManager::instance().update(sql, params, &err);

    if (ret > 0) {
        QMessageBox::information(this, "成功", "删除成功！");
        refreshFlights();
    } else {
        QMessageBox::critical(this, "失败", "删除失败: " + err);
    }
}

void ServerWindow::on_btnShowAddDialog_clicked()
{
    AddFlightDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshFlights();
    }
}

// ---------------------------------------------------------
// 刷新订单 (修复列越界)
// ---------------------------------------------------------
void ServerWindow::refreshOrders()
{
    if(!ui->tableOrders) return;

    QString sql = "SELECT o.id, u.username, f.flight_no, o.passenger_name, o.price_cents, o.status "
                  "FROM orders o "
                  "LEFT JOIN user u ON o.user_id = u.id "
                  "LEFT JOIN flight f ON o.flight_id = f.id "
                  "ORDER BY o.id DESC";

    QSqlQuery query = DBManager::instance().Query(sql);

    // 表头
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

        // 第4列：价格
        int priceCents = query.value(4).toInt();
        double priceYuan = priceCents / 100.0;
        ui->tableOrders->setItem(row, 4, new QTableWidgetItem(QString::number(priceYuan, 'f', 2) + " 元"));

        // 第5列：状态
        int status = query.value(5).toInt();
        QString statusStr = (status == 0) ? "已预订" : (status == 1) ? "已支付" : (status == 2) ? "已取消" : "未知";
        ui->tableOrders->setItem(row, 5, new QTableWidgetItem(statusStr));
    }
}

void ServerWindow::on_btnCancelOrder_clicked()
{
    int row = ui->tableOrders->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一个订单！");
        return;
    }

    QString orderId = ui->tableOrders->item(row, 0)->text();
    QString passenger = ui->tableOrders->item(row, 3)->text();

    if (QMessageBox::question(this, "确认", "确定要强制取消 " + passenger + " 的订单吗？")
        != QMessageBox::Yes) {
        return;
    }

    QString sql = "UPDATE orders SET status = 2 WHERE id = ?";
    QList<QVariant> params;
    params << orderId;

    QString err;
    if (DBManager::instance().update(sql, params, &err) > 0) {
        QMessageBox::information(this, "成功", "订单已取消");
        refreshOrders();
    } else {
        QMessageBox::critical(this, "失败", "操作失败: " + err);
    }
}

void ServerWindow::on_btnShowAddOrderDialog_clicked()
{
    AddOrderDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshOrders();
        refreshFlights();
    }
}

void ServerWindow::on_btnDeleteUser_clicked()
{
    int row = ui->tableAllUsers->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中一个用户！");
        return;
    }

    QString userId = ui->tableAllUsers->item(row, 0)->text();
    QString username = ui->tableAllUsers->item(row, 1)->text();

    if (QMessageBox::question(this, "危险操作",
                              "确定要注销用户 [" + username + "] 吗？\n该操作无法撤销！")
        != QMessageBox::Yes) {
        return;
    }

    QString sql = "DELETE FROM user WHERE id = ?";
    QList<QVariant> params;
    params << userId;

    QString err;
    if (DBManager::instance().update(sql, params, &err) > 0) {
        QMessageBox::information(this, "成功", "用户已删除");
        refreshAllUsers();
        refreshOnlineUsers();
    } else {
        QMessageBox::critical(this, "失败", "删除失败: " + err);
    }
}

void ServerWindow::on_btnShowAddUserDialog_clicked()
{
    AddUserDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshAllUsers();
    }
}
