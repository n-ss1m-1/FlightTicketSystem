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

    QString errMsg;
    //登录
    if (type == Protocol::TYPE_LOGIN) {
        const QString username = data.value("username").toString();
        const QString password = data.value("password").toString();

        qInfo() << "Login request:" << username;

        Common::UserInfo user;


        // 1. 调用查询函数
        DBResult res = db.getUserByUsername(username, user, &errMsg);

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

    //注册
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


        DBResult ret = db.addUser(username,password,phone,realName,idCard,&errMsg);

        if (ret == DBResult::Success) {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_REGISTER_RESP, QJsonObject(), "注册成功"));
        } else {
            qCritical() << "Register DB Error:" << errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "注册失败，数据库错误"));
        }
        return;
    }

    //修改密码
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

        res=db.updatePasswdByUsername(username,newPwd,&errMsg);
        if (res == DBResult::Success) {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_CHANGE_PWD_RESP, QJsonObject(), "密码修改成功"));
        } else {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "密码修改失败"));
        }
        return;
    }
    //根据出发地+目的地+日期查询航班
    else if(type == Protocol::TYPE_FLIGHT_SEARCH)
    {
        const QString fromCity=data.value("fromCity").toString();
        const QString toCity=data.value("toCity").toString();
        const QDate date=QDate::fromString(data.value("date").toString(), Qt::ISODate);

        qInfo() << "search flights request: from" << fromCity << "to" << toCity << "date" << date.toString(Qt::ISODate);

        if (fromCity.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "出发城市不能为空"));
            return;
        }
        if (toCity.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "到达城市不能为空"));
            return;
        }
        if (!date.isValid()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "查询日期格式无效"));
            return;
        }

        //查询航班信息
        QList<Common::FlightInfo> flights;

        DBResult res = db.searchFlights(fromCity,toCity,date,flights,&errMsg);
        if(res == DBResult::Success)
        {
            QJsonArray flightsArr = Common::flightsToJsonArray(flights);
            QJsonObject respData;
            respData.insert("flights",flightsArr);
            respData.insert("count",flights.size());
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_FLIGHT_SEARCH_RESP,respData,QString("航班查询成功,查询到%1条航班").arg(flights.size())));
        }
        else
        {
            qCritical()<<"flight search error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"航班查询失败"+errMsg));
        }
        return;
    }
    //创建订单
    else if(type == Protocol::TYPE_ORDER_CREATE)
    {
        //需要客户端传入：user_id,flight_id,passenger_name,passenger_id_card (可以使用一个user给多个不同的passenger创建订单？)
        Common::OrderInfo order;
        order.userId=data.value("userId").toVariant().toLongLong();
        order.flightId=data.value("flightId").toVariant().toLongLong();
        order.passengerName=data.value("passengerName").toString();
        order.passengerIdCard=data.value("passengerIdCard").toString();
        if (order.userId<=0) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "用户id不能<=0"));
            return;
        }
        if (order.flightId<=0) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "航班id不能<=0"));
            return;
        }
        if (order.passengerName.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "乘客姓名不能为空"));
            return;
        }
        if (order.passengerIdCard.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "乘客IdCard不能为空"));
            return;
        }

        DBResult res=db.createOrder(order,&errMsg);
        if(res == DBResult::Success)
        {
            QJsonObject orderObj = Common::orderToJson(order);
            QJsonObject respData;
            respData.insert("order",orderObj);              //包含order的所有信息
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_ORDER_CREATE_RESP,respData,QString("订单创建成功,订单号：%1").arg(order.id)));
        }
        else
        {
            qCritical()<<"order create error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单创建失败"+errMsg));
        }
        return;
    }
    //查询用户所有订单(根据userId)
    else if(type == Protocol::TYPE_ORDER_LIST)
    {
        const qint64 userId=data.value("userId").toVariant().toLongLong();
        if (userId<=0) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "用户id不能<=0"));
            return;
        }

        QList<Common::OrderInfo> orders;
        DBResult res=db.getOrdersByUserId(userId,orders,&errMsg);
        if(res == DBResult::Success)
        {
            QJsonArray orderArr = Common::ordersToJsonArray(orders);
            QJsonObject respData;
            respData.insert("orders",orderArr);
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_ORDER_LIST_RESP,respData,QString("查询到%1条订单").arg(orders.size())));
        }
        else
        {
            qCritical()<<"order search error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单查询失败"+errMsg));
        }
        return;
    }
    //取消订单(根据userId和orderId) 注意：仅Booked状态的订单可以取消
    else if(type == Protocol::TYPE_ORDER_CANCEL)
    {
        const qint64 userId=data.value("userId").toVariant().toLongLong();
        const qint64 orderId=data.value("orderId").toVariant().toLongLong();
        if (userId<=0) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "用户id不能<=0"));
            return;
        }
        if (orderId<=0) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "orderid不能<=0"));
            return;
        }

        DBResult res=db.cancelOrder(userId,orderId,&errMsg);
        if(res == DBResult::Success)
        {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_ORDER_CANCEL_RESP,QJsonObject(),QString("订单取消成功")));
        }
        else
        {
            qCritical()<<"order cancel error"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单取消失败"+errMsg));
        }
        return;
    }

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
