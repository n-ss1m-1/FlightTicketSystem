#include "ServerWindow.h"
#include "ui_ServerWindow.h"
#include "DBManager.h"
#include "OnlineUserManager.h"
#include "Common/Models.h"
#include "AddFlightDialog.h"
#include "AddOrderDialog.h"
#include "AddUserDialog.h"
#include <QDateTime>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QEventLoop>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>

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
    g_window = this;

    // 安装日志处理器
    qInstallMessageHandler(myMessageOutput);
    connect(this, &ServerWindow::logSignal, this, &ServerWindow::onLogReceived);

    // 连接服务器信号
    connect(m_server, &FlightServer::serverStarted, this, [this]() {
        m_isServerRunning = true;
        updateUIState();
        qInfo() << "服务器启动成功";
    });

    connect(m_server, &FlightServer::serverStopped, this, [this]() {
        m_isServerRunning = false;
        updateUIState();
        qInfo() << "服务器已停止";
    });

    connect(m_server, &FlightServer::serverPaused, this, [this]() {
        updateUIState();
        qInfo() << "服务器已暂停";
    });

    connect(m_server, &FlightServer::serverResumed, this, [this]() {
        updateUIState();
        qInfo() << "服务器已恢复";
    });

    connect(m_server, &FlightServer::clientConnected, this, [this](const QString& clientInfo) {
        qInfo() << "客户端连接:" << clientInfo;
        refreshOnlineUsers();  // 刷新在线用户列表
    });

    connect(m_server, &FlightServer::clientDisconnected, this, [this](const QString& clientInfo) {
        qInfo() << "客户端断开:" << clientInfo;
        refreshOnlineUsers();  // 刷新在线用户列表
    });

    // 初始化UI
    initTables();
    updateUIState();

    // 设置数据库连接 - 使用您提供的DBManager.connect方法
    QString host = "localhost";      // 修改为您的MySQL服务器地址
    int port = 3306;                 // MySQL默认端口
    QString user = "root";           // 修改为您的MySQL用户名
    QString passwd = "passwd";     // 修改为您的MySQL密码
    QString dbName = "flight_ticket";       // 修改为您的数据库名
    QString errMsg;

    qInfo() << "正在连接数据库...";

    // 注意：这里使用 instance() 而不是 getInstance()
    bool connected = DBManager::instance().connect(host, port, user, passwd, dbName, &errMsg);

    if (connected) {
        qInfo() << "数据库连接成功";

        // 测试数据库是否正常工作
        if (DBManager::instance().isConnected()) {
            qInfo() << "数据库连接状态正常";

            // 初始化刷新
            on_btnRefresh_clicked();
        } else {
            QMessageBox::warning(this, "数据库警告", "数据库连接状态异常");
        }
    } else {
        QMessageBox::critical(this, "数据库连接失败",
                              "无法连接到数据库:\n" + errMsg +
                                  "\n\n请检查:\n"
                                  "1. MySQL服务器是否启动\n"
                                  "2. 数据库连接参数是否正确\n"
                                  "3. 防火墙设置\n"
                                  "4. ODBC驱动是否正确安装");
        qCritical() << "数据库连接失败:" << errMsg;
    }
}

ServerWindow::~ServerWindow()
{
    qInstallMessageHandler(nullptr);
    if (m_server) {
        m_server->stop();
    }
    delete ui;
}

void ServerWindow::appendLog(const QString &msg) {
    emit logSignal(msg);
}

void ServerWindow::onLogReceived(const QString &msg) {
    ui->textEditLog->append(msg);

    // 自动滚动到底部
    QTextCursor cursor = ui->textEditLog->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEditLog->setTextCursor(cursor);
}

void ServerWindow::initTables() {
    // 初始化在线用户表
    if(ui->tableOnline) {
        QStringList header; header << "用户名" << "姓名" << "电话" << "状态";
        ui->tableOnline->setColumnCount(header.size());
        ui->tableOnline->setHorizontalHeaderLabels(header);
    }

    // 初始化航班表
    if(ui->tableFlights) {
        QStringList header;
        header << "ID" << "航班号" << "出发" << "到达" << "时间" << "余票" << "价格";
        ui->tableFlights->setColumnCount(header.size());
        ui->tableFlights->setHorizontalHeaderLabels(header);
    }

    // 初始化用户表
    if(ui->tableAllUsers) {
        QStringList header;
        header << "ID" << "用户名" << "真实姓名" << "电话" << "身份证";
        ui->tableAllUsers->setColumnCount(header.size());
        ui->tableAllUsers->setHorizontalHeaderLabels(header);
    }

    // 初始化订单表
    if(ui->tableOrders) {
        QStringList header;
        header << "订单ID" << "下单用户" << "航班号" << "乘客姓名" << "价格" << "状态";
        ui->tableOrders->setColumnCount(header.size());
        ui->tableOrders->setHorizontalHeaderLabels(header);
    }
}

void ServerWindow::updateUIState()
{
    QString statusText;
    QString buttonText = "启动服务器";
    QString pauseButtonText = "暂停";
    bool startButtonEnabled = true;
    bool pauseButtonEnabled = false;
    QString style;

    if (m_server->isRunning()) {
        if (m_server->isPaused()) {
            statusText = "已暂停 (端口: 12345)";
            buttonText = "重启服务器";
            pauseButtonText = "恢复";
            pauseButtonEnabled = true;
            startButtonEnabled = true;
            style = "color: orange; font-weight: bold;";
        } else {
            statusText = "运行中 (端口: 12345)";
            buttonText = "重启服务器";
            pauseButtonText = "暂停";
            pauseButtonEnabled = true;
            startButtonEnabled = true;
            style = "color: green; font-weight: bold;";
        }
    } else {
        statusText = "已停止";
        buttonText = "启动服务器";
        pauseButtonText = "暂停";
        pauseButtonEnabled = false;
        startButtonEnabled = true;
        style = "color: red; font-weight: bold;";
    }

    // 更新UI元素
    ui->labelStatus->setText("服务器状态: " + statusText);
    ui->labelStatus->setStyleSheet(style);
    ui->btnStart->setText(buttonText);
    ui->btnStart->setEnabled(startButtonEnabled);
    ui->btnPause->setText(pauseButtonText);
    ui->btnPause->setEnabled(pauseButtonEnabled);
}

// 启动服务器
void ServerWindow::startServer()
{
    qInfo() << "正在启动服务器...";

    if (m_server->start(12345)) {
        m_isServerRunning = true;
        updateUIState();
        QMessageBox::information(this, "成功", "服务器已启动 (端口: 12345)");
    } else {
        QMessageBox::critical(this, "启动失败",
                              "服务器启动失败:\n"
                              "1. 端口可能被占用\n"
                              "2. 请检查防火墙设置\n"
                              "3. 尝试使用管理员权限运行");
    }
}

// 停止服务器
void ServerWindow::stopServer(bool notifyClients)
{
    if (m_server->isRunning()) {
        m_server->stop();
        m_isServerRunning = false;
    }
}

// 重启服务器
void ServerWindow::restartServer()
{
    qInfo() << "正在重启服务器...";

    // 保存当前状态
    bool wasPaused = m_server->isPaused();

    // 先暂停服务器（通知客户端）
    if (!wasPaused && m_server->isRunning()) {
        m_server->pause();

        // 短暂延迟
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, &QEventLoop::quit);
        loop.exec();
    }

    // 停止服务器
    m_server->stop();

    // 短暂延迟，确保端口释放
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    // 重新启动服务器
    if (m_server->start(12345)) {
        updateUIState();
        qInfo() << "服务器重启成功";
        QMessageBox::information(this, "成功", "服务器重启成功");
    } else {
        qCritical() << "服务器重启失败";
        QMessageBox::critical(this, "重启失败", "服务器重启失败，请检查端口是否被占用");
    }
}

void ServerWindow::on_btnStart_clicked()
{
    if (!m_server->isRunning()) {
        // 服务器未运行，启动服务器
        startServer();
    } else {
        // 服务器正在运行，执行重启
        int result = QMessageBox::question(this, "重启确认",
                                           "服务器正在运行，是否要重启？\n"
                                           "重启将断开所有客户端连接。\n\n"
                                           "选择：\n"
                                           "是 - 重启服务器\n"
                                           "否 - 返回");

        if (result == QMessageBox::Yes) {
            restartServer();
        }
    }
}

void ServerWindow::on_btnPause_clicked()
{
    if (m_server->isRunning() && !m_server->isPaused()) {
        int result = QMessageBox::question(this, "暂停确认",
                                           "确定要暂停服务器吗？\n"
                                           "暂停将断开所有客户端连接并停止接受新连接。\n"
                                           "暂停后可以使用启动按钮重新启动。");

        if (result == QMessageBox::Yes) {
            m_server->pause();
            updateUIState();
        }
    } else if (m_server->isRunning() && m_server->isPaused()) {
        // 如果已暂停，则恢复
        m_server->resume();
        updateUIState();
        QMessageBox::information(this, "成功", "服务器已恢复运行");
    }
}

// 刷新按钮
void ServerWindow::on_btnRefresh_clicked() {
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

void ServerWindow::refreshOrders()
{
    if(!ui->tableOrders) return;

    QString sql = "SELECT o.id, u.username, f.flight_no, o.passenger_name, o.price_cents, o.status "
                  "FROM orders o "
                  "LEFT JOIN user u ON o.user_id = u.id "
                  "LEFT JOIN flight f ON o.flight_id = f.id "
                  "ORDER BY o.id DESC";

    QSqlQuery query = DBManager::instance().Query(sql);

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
        QString statusStr = (status == 0) ? "已预订" : (status == 1) ? "已支付" : (status == 2) ? "已改签" : (status == 3) ? "已取消" : (status == 4) ? "已完成" : "未知";
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

    QString sql = "UPDATE orders SET status = ? WHERE id = ?";
    QList<QVariant> params;
    params << static_cast<int>(Common::OrderStatus::Canceled) << orderId;

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
