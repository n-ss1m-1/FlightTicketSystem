#include "ClientHandler.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QRegularExpression>   //正则表达式
#include "Common/Protocol.h"
#include "DBManager.h"
#include "OnlineUserManager.h"

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , isLogin(false)
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
    DBManager& db = DBManager::instance();
    //获取用户管理的单例
    OnlineUserManager& userManager = OnlineUserManager::instance();

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
        if (res == DBResult::Success && user.password == password)
        {
            this->setUserInfo(user);    //保存用户信息到当前ClientHandler
            isLogin=true;
            emit loginSuccess();        //发送信号 将用户信息保存到server的映射表中

            // 登录成功，把用户信息回传给客户端
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_LOGIN_RESP, Common::userToJson(user), "登录成功"));
        } else
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "账号或密码错误"));
        }
    }
    //退出登陆
    else if(type == Protocol::TYPE_LOGOUT)
    {
        //先判断是否真正登陆
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        //获取用户信息--打印日志
        const QString username=userManager.getUserInfoByHandler(this).username;
        qInfo() << "logout request:" << username;

        //设置状态 清空用户信息 移出用户表
        this->isLogin=false;
        this->m_userInfo=Common::UserInfo();
        userManager.removeOnlineUser(this);

        sendJson(Protocol::makeOkResponse(Protocol::TYPE_LOGOUT_RESP, QJsonObject(), "退出登陆成功"));
    }

    //注册
    else if (type == Protocol::TYPE_REGISTER) {
        const QString username = data.value("username").toString();
        const QString password = data.value("password").toString();
        const QString phone = data.value("phone").toString();
        const QString idCard = data.value("idCard").toString(); // 身份证
        const QString realName = data.value("realName").toString(); // 真实姓名

        if(username.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "用户名不能为空"));
            return;
        }
        if(password.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "密码不能为空"));
            return;
        }
        static const QRegularExpression phoneReg("^1[3-9]\\d{9}$");  //第一位：1  第二位：3~9 后面接9个数字
        if(!phoneReg.match(phone).hasMatch()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "手机号格式无效（第一位:1  第二位:3~9  后面接任意9个数字）"));
            return;
        }
        if(idCard.isEmpty() || idCard.length()!=18 ) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "身份证号必须为18位"));
            return;
        }
        if (realName.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "真实姓名不能为空"));
            return;
        }

        qInfo() << "Register request:" << username;

        // 检查用户是否已存在
        Common::UserInfo existUser;
        if (db.getUserByUsername(username, existUser) == DBResult::Success)
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "注册失败：用户名已存在"));
            return;
        }


        DBResult ret = db.addUser(username,password,phone,realName,idCard,&errMsg);

        if (ret == DBResult::Success)
        {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_REGISTER_RESP, QJsonObject(), "注册成功"));
        } else
        {
            qCritical() << "Register DB Error:" << errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "注册失败:"+errMsg));
        }
    }

    //修改密码
    else if (type == Protocol::TYPE_CHANGE_PWD) {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

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
        if (res == DBResult::Success)
        {
            this->m_userInfo.password=newPwd;   //同步更新ClientHandler中的用户信息
            userManager.addOnlineUser(this);    //覆盖更新在线用户管理表
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_CHANGE_PWD_RESP, QJsonObject(), "密码修改成功"));
        } else
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "密码修改失败:"+errMsg));
        }
    }
    //根据用户名修改电话号码
    else if(type == Protocol::TYPE_CHANGE_PHONE)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        const QString username=data.value("username").toString();
        const QString newPhone=data.value("newPhone").toString();
        if (username.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "用户名不能为空"));
            return;
        }
        if(newPhone.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "新的手机号不能为空"));
            return;
        }
        static const QRegularExpression phoneReg("^1[3-9]\\d{9}$");  //第一位：1  第二位：3~9 后面接9个数字
        if (!phoneReg.match(newPhone).hasMatch()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "手机号格式无效（第一位:1  第二位:3~9  后面接任意9个数字）"));
            return;
        }

        qInfo()<<"change phone request: user:"<<username<<" newPhone:"<<newPhone;

        DBResult res = db.updatePhoneByUsername(username,newPhone,&errMsg);
        if(res == DBResult::Success)
        {
            this->m_userInfo.phone=newPhone;    //同步更新ClientHandler中的用户信息
            userManager.addOnlineUser(this);    //覆盖更新在线用户管理表
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_CHANGE_PHONE_RESP,QJsonObject(),"电话号码修改成功"));
        }
        else
        {
            qCritical()<<"phone change error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"电话号码修改失败:"+errMsg));
        }
    }
    //查询常用乘机人
    else if(type == Protocol::TYPE_PASSENGER_GET)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        Common::UserInfo user=userManager.getUserInfoByHandler(this);
        QList<Common::PassengerInfo> passengers;

        qInfo()<<"Get Passengers request: user:"<<user.username;

        DBResult res=db.getPassengers(user.id,passengers,&errMsg);

        if(res == DBResult::Success)
        {
            QJsonArray passengerArr = Common::passengersToJsonArray(passengers);
            QJsonObject respData;
            respData.insert("passengers",passengerArr);
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_PASSENGER_GET_RESP,respData,QString("查询到%1个常用乘机人").arg(passengers.size())));
        }
        else if(res == DBResult::NoData)
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"暂无常用乘机人:"+errMsg));
        }
        else
        {
            qCritical()<<"passengers get error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"常用乘机人查询失败:"+errMsg));
        }
    }
    //添加常用乘机人 需要传入添加的乘机人的姓名和身份证号
    else if(type == Protocol::TYPE_PASSENGER_ADD)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        Common::UserInfo user=userManager.getUserInfoByHandler(this);
        const QString passenger_name=data.value("passenger_name").toString();
        const QString passenger_id_card=data.value("passenger_id_card").toString().toUpper();

        if (passenger_name.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "常用乘机人姓名不能为空"));
            return;
        }
        if(passenger_id_card.isEmpty() || passenger_id_card.length()!=18 ) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "常用乘机人身份证号必须为18位"));
            return;
        }

        // static const QRegularExpression idCardReg("^[1-9]\\d{5}(18|19|20)\\d{2}(0[1-9]|1[0-2])(0[1-9]|[12]\\d|3[01])\\d{3}[0-9X]$");
        // if (!idCardReg.match(passenger_id_card).hasMatch()) {
        //     sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "身份证号格式无效"));
        //     return;
        // }

        qInfo()<<"Add Passenger request: user:"<<user.username<<" to add passenger:"<<passenger_name<<"("<<passenger_id_card<<")";

        DBResult res=db.addPassenger(user.id,passenger_name,passenger_id_card,&errMsg);
        if(res == DBResult::Success)
        {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_PASSENGER_ADD_RESP,QJsonObject(),QString("添加常用乘机人成功")));
        }
        else
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,QString("添加常用乘机人失败:")+errMsg));
        }

    }
    //删除常用乘机人 需要传入添加的乘机人的姓名和身份证号
    else if(type == Protocol::TYPE_PASSENGER_DEL)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        Common::UserInfo user=userManager.getUserInfoByHandler(this);
        const QString passenger_name=data.value("passenger_name").toString();
        const QString passenger_id_card=data.value("passenger_id_card").toString().toUpper();
        if (passenger_name.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "常用乘机人姓名不能为空"));
            return;
        }
        if(passenger_id_card.isEmpty()) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "常用乘机人身份证号不能为空"));
            return;
        }

        qInfo()<<"Delete Passenger request: user:"<<user.username<<" to delete passenger:"<<passenger_name<<"("<<passenger_id_card<<")";

        DBResult res=db.delPassenger(user.id,passenger_name,passenger_id_card,&errMsg);
        if(res == DBResult::Success)
        {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_PASSENGER_DEL_RESP,QJsonObject(),QString("删除常用乘机人成功")));
        }
        else
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,QString("删除常用乘机人失败:")+errMsg));
        }
    }
    //根据出发地+目的地+日期查询航班
    else if(type == Protocol::TYPE_FLIGHT_SEARCH)
    {
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

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
        else if(res == DBResult::NoData)
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"暂无相关航班:"+errMsg));
        }
        else
        {
            qCritical()<<"flight search error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"航班查询失败:"+errMsg));
        }
    }
    //创建订单
    else if(type == Protocol::TYPE_ORDER_CREATE)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        //需要客户端传入：user_name,flight_id,passenger_name,passenger_id_card (可以使用一个user给多个不同的passenger创建订单？)

        Common::UserInfo user=userManager.getUserInfoByHandler(this);
        const QString username=user.username;
        Common::OrderInfo order;
        order.userId=user.id;
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

        qInfo() << "create order request: from username:" << username << "flightId" << order.flightId << "passengerName" << order.passengerName << "passengerIdCard" <<order.passengerIdCard;

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
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单创建失败:"+errMsg));
        }
    }
    else if(type == Protocol::TYPE_ORDER_PAY)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        const Common::UserInfo user=userManager.getUserInfoByHandler(this);
        const qint64 orderId=data.value("orderId").toVariant().toLongLong();


        qInfo() << "pay for order request: from username:" << user.username;

        DBResult res=db.payForOrder(orderId,&errMsg);
        if(res == DBResult::Success)
        {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_ORDER_PAY_RESP,QJsonObject(),QString("订单(%1)支付成功").arg(orderId)));
        }
        else
        {
            qCritical()<<"pay for order error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单支付失败:"+errMsg));
        }
    }
    //查询用户所有订单(根据userId) --- 已支付订单
    else if(type == Protocol::TYPE_ORDER_LIST)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        const Common::UserInfo user=userManager.getUserInfoByHandler(this);
        const qint64 userId=user.id;


        qInfo() << "search orders that the account holder paid request: from username:" << user.username;

        QList<QPair<Common::OrderInfo,Common::FlightInfo>> ordersAndflights;

        DBResult res=db.getOrdersByUserId(userId,ordersAndflights,&errMsg);

        if(res == DBResult::Success)
        {
            QJsonArray orderAndflightArr = Common::ordersAndflightsToJsonArray(ordersAndflights);
            QJsonObject respData;
            respData.insert("ordersAndflights",orderAndflightArr);
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_ORDER_LIST_RESP,respData,QString("查询到%1条订单").arg(orderAndflightArr.size())));
        }
        else
        {
            qCritical()<<"order search error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单查询失败:"+errMsg));
        }

    }
    //查询该用户的本人订单
    else if(type == Protocol::TYPE_ORDER_LIST_MY)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        const Common::UserInfo user=userManager.getUserInfoByHandler(this);
        const QString realName=user.realName;
        const QString idCard=user.idCard;

        qInfo() << "search orders of the account holder request: from username:" << user.username;

        QList<QPair<Common::OrderInfo,Common::FlightInfo>> ordersAndflights;

        DBResult res=db.getOrdersByRealName(realName,idCard,ordersAndflights,&errMsg);

        if(res == DBResult::Success)
        {
            QJsonArray orderAndflightArr = Common::ordersAndflightsToJsonArray(ordersAndflights);
            QJsonObject respData;
            respData.insert("ordersAndflights",orderAndflightArr);
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_ORDER_LIST_RESP,respData,QString("查询到%1条订单").arg(orderAndflightArr.size())));
        }
        else
        {
            qCritical()<<"order search error:"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单查询失败:"+errMsg));
        }

    }
    //取消订单(根据userId和orderId) 注意：仅Booked状态的订单可以取消
    else if(type == Protocol::TYPE_ORDER_CANCEL)
    {
        //检查用户是否真正登陆 避免非法JSON构造
        if(!isLoggedIn())
        {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "请先登录"));
            return;
        }

        const Common::UserInfo user=userManager.getUserInfoByHandler(this);
        const qint64 orderId=data.value("orderId").toVariant().toLongLong();
        if (orderId<=0) {
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR, "orderid不能<=0"));
            return;
        }

        qInfo() << "cancel order request: from username:" << user.username <<" orderId:" << orderId;

        DBResult res=db.cancelOrder(orderId,&errMsg);
        if(res == DBResult::Success)
        {
            sendJson(Protocol::makeOkResponse(Protocol::TYPE_ORDER_CANCEL_RESP,QJsonObject(),QString("订单取消成功")));
        }
        else
        {
            qCritical()<<"order cancel error"<<errMsg;
            sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"订单取消失败:"+errMsg));
        }
    }

    // 未知请求
    else sendJson(Protocol::makeFailResponse(Protocol::TYPE_ERROR,"Unknown request type: " + type));
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
    qInfo() << "Client disconnected"<<m_socket->peerAddress().toString();

    //从在线用户列表中移除
    OnlineUserManager::instance().removeOnlineUser(this);
    //销毁连接和自身
    m_socket->deleteLater();
    deleteLater();
}
