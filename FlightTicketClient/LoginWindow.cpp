#include "LoginWindow.h"
#include "NetworkManager.h"

#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QJsonObject>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    // 连接 NetworkManager 的信号
    auto net = NetworkManager::instance();
    connect(net, &NetworkManager::connected,
            this, &LoginWindow::onConnected);
    connect(net, &NetworkManager::disconnected,
            this, &LoginWindow::onDisconnected);
    connect(net, &NetworkManager::jsonReceived,
            this, &LoginWindow::onJsonReceived);
    connect(net, &NetworkManager::errorOccurred,
            this, &LoginWindow::onError);
}

void LoginWindow::setupUi()
{
    setWindowTitle("Flight Client - Login");

    m_editHost = new QLineEdit("127.0.0.1");
    m_editPort = new QLineEdit("12345");
    m_editPort->setValidator(new QIntValidator(1, 65535, this));

    m_editUser = new QLineEdit;
    m_editPass = new QLineEdit;
    m_editPass->setEchoMode(QLineEdit::Password);

    m_btnLogin = new QPushButton("连接并登录");
    m_labelStatus = new QLabel("未连接");

    auto layout = new QVBoxLayout(this);

    auto rowHost = new QHBoxLayout;
    rowHost->addWidget(new QLabel("服务器IP:"));
    rowHost->addWidget(m_editHost);

    auto rowPort = new QHBoxLayout;
    rowPort->addWidget(new QLabel("端口:"));
    rowPort->addWidget(m_editPort);

    auto rowUser = new QHBoxLayout;
    rowUser->addWidget(new QLabel("用户名:"));
    rowUser->addWidget(m_editUser);

    auto rowPass = new QHBoxLayout;
    rowPass->addWidget(new QLabel("密码:"));
    rowPass->addWidget(m_editPass);

    layout->addLayout(rowHost);
    layout->addLayout(rowPort);
    layout->addLayout(rowUser);
    layout->addLayout(rowPass);
    layout->addWidget(m_btnLogin);
    layout->addWidget(m_labelStatus);

    connect(m_btnLogin, &QPushButton::clicked,
            this, &LoginWindow::onLoginClicked);
}

void LoginWindow::onLoginClicked()
{
    auto net = NetworkManager::instance();
    if (!net->isConnected()) {
        QString host = m_editHost->text();
        quint16 port = m_editPort->text().toUShort();
        m_labelStatus->setText("正在连接...");
        net->connectToServer(host, port);
    } else {
        sendLoginRequest();
    }
}

void LoginWindow::onConnected()
{
    m_labelStatus->setText("已连接服务器，发送登录请求...");
    sendLoginRequest();
}

void LoginWindow::onDisconnected()
{
    m_labelStatus->setText("连接已断开");
}

void LoginWindow::onJsonReceived(const QJsonObject &obj)
{
    QString type = obj.value("type").toString();
    if (type == "login_response") {
        bool success = obj.value("success").toBool();
        QString msg = obj.value("message").toString();
        if (success) {
            QMessageBox::information(this, "登录结果", "登录成功：" + msg);
            // 这里未来可以打开主界面 MainWindow
        } else {
            QMessageBox::warning(this, "登录结果", "登录失败：" + msg);
        }
    }
}

void LoginWindow::onError(const QString &msg)
{
    m_labelStatus->setText("错误：" + msg);
}

void LoginWindow::sendLoginRequest()
{
    QString user = m_editUser->text();
    QString pass = m_editPass->text();

    QJsonObject data{
        {"username", user},
        {"password", pass}
    };
    QJsonObject obj{
        {"type", "login"},
        {"data", data}
    };
    NetworkManager::instance()->sendJson(obj);
}
