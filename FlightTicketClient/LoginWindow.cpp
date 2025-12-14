#include "LoginWindow.h"
#include "NetworkManager.h"

#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QMessageBox>
#include <QJsonObject>

#include "Common/Protocol.h"

static const char* SERVER_HOST = "127.0.0.1";
static const quint16 SERVER_PORT = 12345;

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    auto net = NetworkManager::instance();
    connect(net, &NetworkManager::connected, this, &LoginWindow::onConnected);
    connect(net, &NetworkManager::disconnected, this, &LoginWindow::onDisconnected);
    connect(net, &NetworkManager::jsonReceived, this, &LoginWindow::onJsonReceived);
    connect(net, &NetworkManager::errorOccurred, this, &LoginWindow::onError);
}

void LoginWindow::setupUi()
{
    setWindowTitle("Flight Client - Login");

    m_editUser = new QLineEdit;
    m_editPass = new QLineEdit;
    m_editPass->setEchoMode(QLineEdit::Password);

    m_btnLogin = new QPushButton("连接并登录");
    m_labelStatus = new QLabel("未连接");

    auto layout = new QVBoxLayout(this);

    auto rowUser = new QHBoxLayout;
    rowUser->addWidget(new QLabel("用户名:"));
    rowUser->addWidget(m_editUser);

    auto rowPass = new QHBoxLayout;
    rowPass->addWidget(new QLabel("密码:"));
    rowPass->addWidget(m_editPass);

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
        m_labelStatus->setText("正在连接...");
        net->connectToServer(SERVER_HOST, SERVER_PORT);
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
    const QString type = obj.value(Protocol::KEY_TYPE).toString();

    if (type == Protocol::TYPE_LOGIN_RESP) {
        const bool success = obj.value(Protocol::KEY_SUCCESS).toBool();
        const QString msg = obj.value(Protocol::KEY_MESSAGE).toString();

        if (success) {
            QMessageBox::information(this, "登录结果", "登录成功：" + msg);
            // TODO: 打开主界面 MainWindow
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
    const QString user = m_editUser->text();
    const QString pass = m_editPass->text();

    QJsonObject data{
        {"username", user},
        {"password", pass}
    };

    QJsonObject req{
        {Protocol::KEY_TYPE, Protocol::TYPE_LOGIN},
        {Protocol::KEY_DATA, data}
    };

    NetworkManager::instance()->sendJson(req);
}
