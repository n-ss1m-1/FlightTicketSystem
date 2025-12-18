#include "ClientHandler.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include "Common/Protocol.h"
#include "DBManager.h"  // <--- 引入队友的数据库管理类

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
}

void ClientHandler::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    processBuffer();
}

void ClientHandler::processBuffer()
{
    while (true) {
        int idx = m_buffer.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            handleJson(doc.object());
        } else {
            qWarning() << "JSON parse error from client:" << err.errorString();
        }
    }
}

void ClientHandler::handleJson(const QJsonObject &obj)
{
    const QString type = obj.value(Protocol::KEY_TYPE).toString();
    const QJsonObject data = obj.value(Protocol::KEY_DATA).toObject();

    // 获取数据库单例
    DBManager &db = DBManager::instance();

    // ==================== 1. 登录 ====================
    if (type == Protocol::TYPE_LOGIN) {
        const QString username = data.value("username").toString();
        const QString password = data.value("password").toString();

        qInfo() << "Login request:" << username;

        Common::UserInfo user;
        QString errorMsg;

        // 1. 调用队友写的查询函数
        DBResult res = db.getUserByUsername(username, user, &errorMsg);

        // 2. 校验结果 (DBResult::Success 且 密码匹配)
        // 注意：这里假设数据库里存的是明文密码，实际开发通常要加密，但作业可以直接比对
        if (res == DBResult::Success && user.password == password) {
            // 登录成功，把用户信息回传给客户端
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_LOGIN_RESP, Common::userToJson(user), "登录成功"));
        } else {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "账号或密码错误"));
        }
        return;
    }

    // ==================== 2. 注册 ====================
    else if (type == Protocol::TYPE_REGISTER) {
        const QString username = data.value("username").toString();
        const QString password = data.value("password").toString();
        const QString phone = data.value("phone").toString();
        const QString idCard = data.value("idCard").toString(); // 身份证
        const QString realName = data.value("realName").toString(); // 真实姓名

        qInfo() << "Register request:" << username;

        // 检查用户是否已存在
        Common::UserInfo existUser;
        if (db.getUserByUsername(username, existUser) == DBResult::Success) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "注册失败：用户名已存在"));
            return;
        }

        QString errMsg;
        DBResult ret = db.addUser(username,password,phone,realName,idCard,&errMsg);

        if (ret == DBResult::Success) {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_REGISTER_RESP, QJsonObject(), "注册成功"));
        } else {
            qCritical() << "Register DB Error:" << errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "注册失败，数据库错误"));
        }
        return;
    }

    // ==================== 3. 修改密码 ====================
    else if (type == Protocol::TYPE_CHANGE_PWD) {
        const QString username = data.value("username").toString();
        const QString oldPwd = data.value("oldPassword").toString();
        const QString newPwd = data.value("newPassword").toString();

        qInfo() << "Change Pwd request:" << username;

        // 1. 先验证旧密码
        Common::UserInfo user;
        DBResult res = db.getUserByUsername(username, user);

        if (res != DBResult::Success || user.password != oldPwd) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "原密码错误"));
            return;
        }

        // 2. 更新新密码
        QString errMsg;
        res=db.updatePasswdByUsername(username,newPwd,&errMsg);
        if (res == DBResult::Success) {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_CHANGE_PWD_RESP, QJsonObject(), "密码修改成功"));
        } else {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "密码修改失败"));
        }
        return;
    }

    // ... 其他逻辑 (如 UPDATE_INFO) 可以依样画葫芦 ...

    // 未知请求
    sendJson(Protocol::makeFailResponse(
        Protocol::TYPE_ERROR,
        "Unknown request type: " + type
        ));
}

void ClientHandler::sendJson(const QJsonObject &obj)
{
    if (!m_socket) return;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
}

void ClientHandler::onDisconnected()
{
    qInfo() << "Client disconnected";
    m_socket->deleteLater();
    deleteLater();
}
