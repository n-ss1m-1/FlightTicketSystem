#include "DBManager.h"
#include <QDebug>


DBManager::DBManager()
{
    //初始化连接
    db=QSqlDatabase::addDatabase("QODBC");
}

DBManager::~DBManager()
{
    //关闭连接
    if(db.isOpen())
    {
        db.close();
    }
}

DBManager& DBManager::instance()   //获取数据库连接对象
{
    static DBManager inst;      //静态变量：只初始化一次+全局生命周期
    return inst;                //返回引用
}

//连接数据库
bool DBManager::connect(const QString& host,int port,const QString& user,const QString& passwd,const QString& dbName,QString* errMsg)
{
    if(db.isOpen())
    {
        db.close();
    }

    // db.setHostName(host);
    // db.setPort(port);
    // db.setDatabaseName(dbName);
    // db.setUserName(user);
    // db.setPassword(passwd);

    QStringList driverCandidates = {
        "MySQL ODBC 8.0 Unicode Driver",
        "MySQL ODBC 9.5 Unicode Driver"
    };

    bool ok = false;
    QString lastErr;

    for (const QString &drv : driverCandidates) {
        QString conn = QString(
                           "Driver={%1};"
                           "Server=%2;"
                           "Port=%3;"
                           "Database=%4;"
                           "User=%5;"
                           "Password=%6;"
                           "Option=3;"
                           ).arg(drv, host, QString::number(port), dbName, user, passwd);

        db.setDatabaseName(conn);
        ok = db.open();
        if (ok) break;
        lastErr = db.lastError().text();
    }

    if (!ok && errMsg) *errMsg = lastErr;
    return ok;
}


bool DBManager::isConnected() const
{
    return db.isOpen() && db.isValid();
}

//查询操作
QSqlQuery DBManager::Query(const QString& sql,const QList<QVariant>& params,QString* errMsg)
{
    if(!isConnected())
    {
        if(errMsg) *errMsg="数据库未连接";
        qWarning()<<"Query失败：数据库未连接";
        return QSqlQuery(db);
    }

    QSqlQuery query(db);
    //预编译sql
    if(!query.prepare(sql))     //prepare失败
    {
        if(errMsg) *errMsg="SQL prepare failed: "+query.lastError().text();
        qWarning()<<"SQL prepare failed: "<<sql<<"Error:"<<query.lastError().text();
        return query;
    }

    for(int i=0; i<params.size();i++)
    {
        query.bindValue(i, params[i]);  //位置绑定
    }

    if(!query.exec())
    {
        if(errMsg)
        {
            *errMsg="sql exec failed: "+query.lastError().text();
            qWarning()<<"sql exec failed: "<<sql<<"Error:"<<query.lastError().text();
        }
    }
    return query;
}


//增删改操作 返回受影响的行数
int DBManager::update(const QString& sql,const QList<QVariant>& params,QString* errMsg)
{
    QSqlQuery query=Query(sql,params,errMsg);
    return query.numRowsAffected();
}


//事务操作
bool DBManager::beginTransaction()
{
    if(!isConnected())
    {
        qWarning()<<"开启事务失败：数据库未连接";
        return false;
    }
    return db.transaction();
}
bool DBManager::commitTransaction()
{
    if(!isConnected())
    {
        qWarning()<<"提交事务失败：数据库未连接";
        return false;
    }
    return db.commit();
}
bool DBManager::rollbackTransaction()
{
    if(!isConnected())
    {
        qWarning()<<"事务回滚失败：数据库未连接";
        return false;
    }
    return db.rollback();
}


//将查询结果转换为相应的Info
Common::UserInfo DBManager::userFromQuery(const QSqlQuery& query,const QString prefix)
{
    Common::UserInfo user;
    user.id=query.value("id").toLongLong();
    user.username=query.value("username").toString();
    user.password=query.value("password").toString();
    user.phone=query.value("phone").toString();
    user.realName=query.value("real_name").toString();
    user.idCard=query.value("id_card").toString();

    return user;
}
Common::PassengerInfo DBManager::passengerFromQuery(const QSqlQuery& query,const QString prefix)
{
    Common::PassengerInfo passenger;
    passenger.id=query.value("id").toLongLong();
    passenger.user_id=query.value("user_id").toLongLong();
    passenger.name=query.value("name").toString();
    passenger.idCard=query.value("id_card").toString();
    return passenger;
}
Common::FlightInfo DBManager::flightFromQuery(const QSqlQuery& query,const QString prefix)
{
    Common::FlightInfo flight;
    QString idField = prefix + "id";
    QString flightNoField = prefix + "flight_no";
    QString fromCityField = prefix + "from_city";
    QString toCityField = prefix + "to_city";
    QString departTimeField = prefix + "depart_time";
    QString arriveTimeField = prefix + "arrive_time";
    QString priceCentsField = prefix + "price_cents";
    QString seatTotalField = prefix + "seat_total";
    QString seatLeftField = prefix + "seat_left";
    QString statusField = prefix + "status";

    flight.id = query.value(idField).toLongLong();
    flight.flightNo = query.value(flightNoField).toString();
    flight.fromCity = query.value(fromCityField).toString();
    flight.toCity = query.value(toCityField).toString();
    flight.departTime = query.value(departTimeField).toDateTime();
    flight.arriveTime = query.value(arriveTimeField).toDateTime();
    flight.priceCents = query.value(priceCentsField).toInt();
    flight.seatTotal = query.value(seatTotalField).toInt();
    flight.seatLeft = query.value(seatLeftField).toInt();
    flight.status = static_cast<Common::FlightStatus>(query.value(statusField).toInt());

    return flight;
}
Common::OrderInfo DBManager::orderFromQuery(const QSqlQuery& query,const QString prefix)
{
    Common::OrderInfo order;

    QString idField = prefix + "id";
    QString userIdField = prefix + "user_id";
    QString flightIdField = prefix + "flight_id";
    QString passengerNameField = prefix + "passenger_name";
    QString passengerIdCardField = prefix + "passenger_id_card";
    QString seatNumField = prefix + "seat_num";
    QString priceCentsField = prefix + "price_cents";
    QString pendingPaymentField = prefix + "pending_payment";
    QString statusField = prefix + "status";
    QString createdTimeField = prefix + "created_time";

    order.id = query.value(idField).toLongLong();
    order.userId = query.value(userIdField).toLongLong();
    order.flightId = query.value(flightIdField).toLongLong();
    order.passengerName = query.value(passengerNameField).toString();
    order.passengerIdCard = query.value(passengerIdCardField).toString();
    order.seatNum = query.value(seatNumField).toString();
    order.priceCents = query.value(priceCentsField).toInt();
    order.pendingPayment = query.value(pendingPaymentField).toInt();
    order.status = static_cast<Common::OrderStatus>(query.value(statusField).toInt());
    order.createdTime = query.value(createdTimeField).toDateTime();

    return order;
}


//封装与业务相关的数据库操作
//用户                    //user作为传出参数
DBResult DBManager::getUserByUsername(const QString& username,Common::UserInfo& user,QString* errMsg)
{
    //无参sql
    QString sql=QString("select * from user where username=?");

    //输入参数
    QList<QVariant> params;
    params<<username;

    //执行查询
    QSqlQuery query=Query(sql,params,errMsg);

    //检查结果
    if(!query.isActive()) return DBResult::QueryFailed;
    if(!query.next()) return DBResult::NoData;

    user=userFromQuery(query);

    return DBResult::Success;
}
DBResult DBManager::getUserById(qint64 userId,Common::UserInfo& user,QString* errMsg)
{
    QString sql="select * from user where id=?";
    QList<QVariant>params;
    params<<userId;

    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive()) return DBResult::QueryFailed;
    if(!query.next()) return DBResult::NoData;

    user=userFromQuery(query);
    return DBResult::Success;
}
// 实现：根据手机号查询用户是否存在（复用现有Query()方法）
DBResult DBManager::getUserByPhone(const QString& phone, Common::UserInfo& existUser, QString* errMsg)
{
    //构造查询SQL
    QString sql = "select * from user where phone = ? limit 1";
    QList<QVariant> params;
    params << phone;

    //执行
    QSqlQuery query = this->Query(sql, params, errMsg);
    //检查Query()执行是否出现错误
    if (errMsg && !errMsg->isEmpty()) {
        qCritical() << "查询手机号失败：" << *errMsg;
        return DBResult::QueryFailed;
    }

    //判断查询结果是否存在(存在则表示手机号已被注册)
    if (query.next()) {
        existUser=userFromQuery(query);
        return DBResult::Success;
    }

    //无查询结果(手机号未被注册)
    return DBResult::QueryFailed;
}

// 实现：根据身份证号查询用户是否存在(复用现有Query()方法)
DBResult DBManager::getUserByIdCard(const QString& id_card, Common::UserInfo& existUser, QString* errMsg)
{
    //构造查询SQL(limit 1 提升查询效率,只需确认是否存在即可)
    QString sql = "select * from user where id_card = ? limit 1";
    QList<QVariant> params;
    params << id_card;

    //执行查询
    QSqlQuery query = this->Query(sql, params, errMsg);
    //检查Query()执行是否出现错误
    if (errMsg && !errMsg->isEmpty()) {
        qCritical() << "查询身份证号失败：" << *errMsg;
        return DBResult::QueryFailed;
    }

    //判断查询结果是否存在(存在则表示身份证已被注册)
    if (query.next()) {
        existUser=userFromQuery(query);
        return DBResult::Success;
    }

    // 5. 无查询结果(身份证未被注册)
    return DBResult::QueryFailed;
}
DBResult DBManager::addUser(const QString& username,const QString& password,const QString& phone,const QString& real_name,const QString& id_card,QString* errMsg)
{
    QString sql="insert into user (username,password,phone,real_name,id_card) VALUES (?, ?, ?, ?, ?)";
    QList<QVariant>params;
    params<<username<<password<<phone<<real_name<<id_card;

    int sqlAffected=update(sql,params,errMsg);
    if(sqlAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+"添加用户失败";
        return DBResult::updateFailed;
    }
    return DBResult::Success;
}
DBResult DBManager::updatePasswdByUsername(const QString& username,const QString& newPasswd,QString* errMsg)
{
    QString sql="update user set password=? WHERE username=?";
    QList<QVariant> params;
    params<<newPasswd<<username;

    int sqlAffected=update(sql,params,errMsg);
    if(sqlAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+"密码更改失败";
        return DBResult::updateFailed;
    }
    return DBResult::Success;
}
DBResult DBManager::updatePhoneByUsername(const QString& username,const QString& phone,QString* errMsg)
{
    QString sql="update user set phone=? where username=?";
    QList<QVariant> params;
    params<<phone<<username;

    int sqlAffected=update(sql,params,errMsg);
    if(sqlAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+"电话号码更改失败";
        return DBResult::updateFailed;
    }
    return DBResult::Success;
}
//常用乘机人
DBResult DBManager::addPassenger(const qint64 user_id,const QString& passenger_name,const QString& passenger_id_card,QString* errMsg)
{
    //检查是否已有该常用乘机人
    QString checkSql="select * from passenger where user_id=? and name=? and id_card=?";
    QList<QVariant> checkParams;
    checkParams<<user_id<<passenger_name<<passenger_id_card;
    QSqlQuery query=Query(checkSql,checkParams,errMsg);
    if(query.isActive() && query.next())
    {
        if(errMsg) *errMsg=*errMsg+"已有该常用乘机人,无需重复添加";
        return DBResult::updateFailed;
    }

    //插入乘机人表
    QString sql="insert into passenger (user_id,name,id_card) values(?,?,?)";
    QList<QVariant> params;
    params<<user_id<<passenger_name<<passenger_id_card;

    int sqlAffected=update(sql,params,errMsg);
    if(sqlAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+"添加常用乘机人失败";
        return DBResult::updateFailed;
    }

    return DBResult::Success;
}
DBResult  DBManager::delPassenger(const qint64 user_id,const QString& passenger_name,const QString& passenger_id_card,QString* errMsg)
{
    QString sql="delete from passenger where user_id=? and name=? and id_card=? ";
    QList<QVariant> params;
    params<<user_id<<passenger_name<<passenger_id_card;

    int sqlAffected=update(sql,params,errMsg);
    if(sqlAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+"删除常用乘机人失败";
        return DBResult::updateFailed;
    }

    return DBResult::Success;
}
DBResult DBManager::getPassengers(const qint64 user_id,QList<Common::PassengerInfo>& passengers,QString* errMsg)
{
    QString sql="select * from passenger where user_id=?";
    QList<QVariant>params;
    params<<user_id;

    //执行sql
    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive())
    {
        return DBResult::QueryFailed;
    }

    //遍历结果集
    passengers.clear();
    while(query.next())     //初始位置：-1
    {
        passengers.append(passengerFromQuery(query));
    }

    return passengers.isEmpty()? DBResult::NoData : DBResult::Success;
}

DBResult DBManager::searchFlights(const Common::FlightQueryCondition& cond,QList<Common::FlightInfo>& flights, QString* errMsg)
{
    QString sql="select * from flight";
    QList<QVariant> params;
    QStringList whereClauses;   //存储查询条件(where)

    //动态拼接查询条件
    if(cond.id!=0)
    {
        whereClauses.append("id=?");
        params.append(cond.id);
    }
    if(!cond.fromCity.isEmpty())
    {
        whereClauses.append("from_city=?");
        params.append(cond.fromCity);
    }
    if(!cond.toCity.isEmpty())
    {
        whereClauses.append("to_city=?");
        params.append(cond.toCity);
    }
    if(cond.maxDepartDate.isValid())
    {
        whereClauses.append("date(depart_time)>=?");
        params.append(cond.minDepartDate.toString("yyyy-MM-dd"));
    }
    if(cond.minDepartDate.isValid())
    {
        whereClauses.append("date(depart_time)<=?");
        params.append(cond.minDepartDate.toString("yyyy-MM-dd"));
    }
    if(cond.minDepartTime.isValid())
    {
        whereClauses.append("time(depart_time)>=?");
        params.append(cond.minDepartTime.toString("HH:mm"));
    }
    if(cond.maxDepartTime.isValid())
    {
        whereClauses.append("time(depart_time)<=?");
        params.append(cond.maxDepartTime.toString("HH:mm"));
    }
    if(cond.minPriceCents>0)
    {
        whereClauses.append("price_cents>=?");
        params.append(cond.minPriceCents);
    }
    if(cond.maxPriceCents>0 && cond.maxPriceCents>=cond.minPriceCents)
    {
        whereClauses.append("price_cents<=?");
        params.append(cond.maxPriceCents);
    }
    whereClauses.append("seat_left>0");

    //拼接完整SQL
    if(!whereClauses.isEmpty())  sql+=" where " + whereClauses.join(" and ");

    //添加：按起飞时间升序排列
    sql+=" order by depart_time asc ";

    qInfo() << "(查询航班)拼接后的SQL：" << sql;
    qInfo() << "查询参数：" << params;

    //执行sql
    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive())
    {
        if(errMsg) *errMsg=*errMsg+" 航班查询失败";
        return DBResult::QueryFailed;
    }

    //解析结果
    flights.clear();
    flights.reserve(query.size());

    while(query.next())
    {
        flights.append(flightFromQuery(query));
    }

    return flights.isEmpty()?DBResult::NoData : DBResult::Success;
}

//获取城市列表
DBResult DBManager::getCityList(QList<QString>& fromCities,QList<QString>& toCities,QString* errMsg)
{
    QString fromSql="select distinct from_city from flight";
    QList<QVariant>params;
    QSqlQuery fromQuery=Query(fromSql,params,errMsg);
    if(!fromQuery.isActive()) return DBResult::QueryFailed;
    while(fromQuery.next())     //初始位置：-1
    {
        fromCities.append(fromQuery.value("from_city").toString());
    }

    QString toSql="select distinct to_city from flight";
    QSqlQuery toQuery=Query(toSql,params,errMsg);
    if(!toQuery.isActive()) return DBResult::QueryFailed;
    while(toQuery.next())     //初始位置：-1
    {
        toCities.append(toQuery.value("to_city").toString());
    }

    if(fromCities.empty() && toCities.empty())
    {
        if(errMsg) *errMsg=*errMsg+"未查询到数据";
        return DBResult::NoData;
    }

    return DBResult::Success;
}

//订单
//创建订单
DBResult DBManager::createOrder(Common::OrderInfo& order,bool autoManageTransaction,QString* errMsg)
{
    //开启事务
    if(autoManageTransaction && !beginTransaction())
    {
        if(errMsg) *errMsg="开启事务失败";
        return DBResult::TransactionFailed;
    }

    //1.减少剩余座位数
    QString seatSql="update flight set seat_left=seat_left-1 where id=? and seat_left>0";
    QList<QVariant> seatParams;
    seatParams<<order.flightId;
    int seatAffected=update(seatSql,seatParams,errMsg);

    //座位不足 or 更新失败
    if(seatAffected<=0)
    {
        if(autoManageTransaction) rollbackTransaction();
        if(errMsg)
        {
            QString detail=!(*errMsg).isEmpty()?*errMsg:"无可用座位";
            *errMsg="航班座位不足或更新失败: "+detail;
        }
        return DBResult::QueryFailed;
    }

    //2.通过flightId查询航班
    QList<Common::FlightInfo> flights;
    Common::FlightQueryCondition cond;
    cond.id=order.flightId;

    if(searchFlights(cond,flights,errMsg)!=DBResult::Success)
    {
        if(autoManageTransaction) rollbackTransaction();
        if (errMsg) *errMsg = "获取航班信息失败: " + *errMsg;
        return DBResult::QueryFailed;
    }
    Common::FlightInfo flight=flights[0];

    //3.更新订单座位
    order.seatNum=QString::number(flight.seatTotal-flight.seatLeft+1);

    //4.初始化订单价格和待支付金额
    order.priceCents=order.pendingPayment=flight.priceCents;

    //5.初始化订单状态：0 Booked
    order.status=Common::OrderStatus::Booked;

    //6.插入订单数据
    QString orderSql="insert into orders (user_id,flight_id,passenger_name,passenger_id_card,seat_num,price_cents,pending_payment,status) values(?,?,?,?,?,?,?,?)";
    QList<QVariant>orderParams;
    orderParams<<order.userId<<order.flightId<<order.passengerName<<order.passengerIdCard<<order.seatNum<<order.priceCents<<order.pendingPayment<<static_cast<int>(order.status);

    QSqlQuery orderQuery=Query(orderSql,orderParams,errMsg);
    if(!orderQuery.isActive())
    {
        if(autoManageTransaction) rollbackTransaction();
        return DBResult::QueryFailed;
    }
    //7.获取新订单ID
    order.id=orderQuery.lastInsertId().toLongLong();

    //8.提交事务/中途事务回滚
    if(autoManageTransaction && !commitTransaction())
    {
        if(autoManageTransaction) rollbackTransaction();
        if(errMsg) *errMsg="提交事务失败";
        return DBResult::TransactionFailed;
    }
    return DBResult::Success;
}
DBResult DBManager::getOrderByFlightId(const qint64 flightId,const QString& passengerName,const QString& passengerIdCard,Common::OrderInfo& existOrder,QString* errMsg)
{
    //构造查询SQL(limit 1 提升查询效率,只需确认是否存在即可)
    QString sql = "select * from orders where flight_id = ? and passenger_name = ? and passenger_id_card = ? limit 1";
    QList<QVariant> params;
    params << flightId << passengerName << passengerIdCard;

    //执行查询
    QSqlQuery query = this->Query(sql, params, errMsg);
    //检查Query()执行是否出现错误
    if (errMsg && !errMsg->isEmpty()) {
        qCritical() << "通过flight_id+passenger_name+passenger_id_card查询订单失败：" << *errMsg;
        return DBResult::QueryFailed;
    }

    //判断查询结果是否存在(存在则表示该用户已对该航班下过订单)
    if (query.next()) {
        existOrder=orderFromQuery(query);   //返回该order
        return DBResult::Success;
    }

    // 5. 无查询结果(该用户未下单过相关航班)
    return DBResult::QueryFailed;
}
DBResult DBManager::payForOrder(qint64 orderId,QString* errMsg)
{
    //修改订单状态->Paid + 修改待支付金额->0
    QString sql="update orders o set o.status=? , o.pending_payment=0 where o.id=?";
    QList<QVariant> params;
    params<<static_cast<int>(Common::OrderStatus::Paid)<<orderId;

    int affected=update(sql,params,errMsg);

    if(affected<=0)
    {
        if(errMsg) *errMsg=*errMsg+" 支付失败";
        return DBResult::updateFailed;
    }

    return DBResult::Success;
}
DBResult DBManager::getOrdersByUserId(qint64 userId,QList<QPair<Common::OrderInfo,Common::FlightInfo>>& ordersAndflights,QString* errMsg)   //已支付订单
{
    //sql语句和参数
    QString sql="select "
                "o.id AS o_id, o.user_id AS o_user_id, o.flight_id AS o_flight_id,"
                "o.passenger_name AS o_passenger_name, o.passenger_id_card AS o_passenger_id_card,"
                "o.seat_num AS o_seat_num, o.price_cents AS o_price_cents, o.pending_payment AS o_pending_payment,"
                "o.status AS o_status, o.created_time AS o_created_time,"
                "f.id AS f_id, f.flight_no AS f_flight_no, f.from_city AS f_from_city,"
                "f.to_city AS f_to_city, f.depart_time AS f_depart_time, f.arrive_time AS f_arrive_time,"
                "f.price_cents AS f_price_cents, f.seat_total AS f_seat_total, f.seat_left AS f_seat_left,"
                "f.status AS f_status "
                "from orders o inner join flight f on o.flight_id=f.id where o.user_id=? order by o.id desc";
    QList<QVariant>params;
    params<<userId;

    //执行sql
    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive())
    {
        return DBResult::QueryFailed;
    }

    //遍历结果集
    ordersAndflights.clear();
    Common::OrderInfo order;
    Common::FlightInfo flight;
    while(query.next())     //初始位置：-1
    {
        order = orderFromQuery(query,"o_");
        flight = flightFromQuery(query,"f_");
        ordersAndflights.append(QPair<Common::OrderInfo, Common::FlightInfo>(order,flight));
    }

    return ordersAndflights.isEmpty()? DBResult::NoData : DBResult::Success;
}
DBResult DBManager::getOrdersByRealName(const QString& realName,const QString& idCard,QList<QPair<Common::OrderInfo,Common::FlightInfo>>& ordersAndflights,QString* errMsg)     //本人订单
{
    //sql语句和参数
    QString sql="select "
                  "o.id AS o_id, o.user_id AS o_user_id, o.flight_id AS o_flight_id,"
                  "o.passenger_name AS o_passenger_name, o.passenger_id_card AS o_passenger_id_card,"
                  "o.seat_num AS o_seat_num, o.price_cents AS o_price_cents, o.pending_payment AS o_pending_payment,"
                  "o.status AS o_status, o.created_time AS o_created_time,"
                  "f.id AS f_id, f.flight_no AS f_flight_no, f.from_city AS f_from_city,"
                  "f.to_city AS f_to_city, f.depart_time AS f_depart_time, f.arrive_time AS f_arrive_time,"
                  "f.price_cents AS f_price_cents, f.seat_total AS f_seat_total, f.seat_left AS f_seat_left,"
                  "f.status AS f_status "
                  "from orders o inner join flight f on o.flight_id=f.id where passenger_name=? and passenger_id_card=? order by o.id desc";
    QList<QVariant>params;
    params<<realName<<idCard;

    //执行sql
    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive())
    {
        return DBResult::QueryFailed;
    }

    //遍历结果集
    ordersAndflights.clear();
    Common::OrderInfo order;
    Common::FlightInfo flight;
    while(query.next())     //初始位置：-1
    {
        order = orderFromQuery(query,"o_");
        flight = flightFromQuery(query,"f_");
        ordersAndflights.append(QPair<Common::OrderInfo, Common::FlightInfo>(order,flight));
    }

    return ordersAndflights.isEmpty()? DBResult::NoData : DBResult::Success;
}
//改签
DBResult DBManager::rescheduleOrder(Common::OrderInfo& oriOrder,Common::OrderInfo& newOrder,qint32& priceDif,QString* errMsg)
{
    //开启事务
    if(!beginTransaction())
    {
        if(errMsg) *errMsg="开启事务失败";
        return DBResult::TransactionFailed;
    }

    //已支付金额=原价-待支付金额
    qint32 oriPaidAmount=oriOrder.priceCents-oriOrder.pendingPayment;

    //取消订单
    DBResult res=this->cancelOrder(oriOrder.id,false,errMsg);
    if(res != DBResult::Success)
    {
        if(errMsg) *errMsg=*errMsg+" 取消订单失败";
        rollbackTransaction();
        return res;
    }

    //创建订单
    res=this->createOrder(newOrder,false,errMsg);
    if(res != DBResult::Success)
    {
        if(errMsg) *errMsg=*errMsg+" 创建新订单失败";
        rollbackTransaction();
        return res;
    }

    //更新原订单状态为Rescheduled
    oriOrder.status = Common::OrderStatus::Rescheduled;
    QString updateStatusSql1 = "update orders set status=? where id=?";
    QList<QVariant> statusParams1;
    statusParams1 << static_cast<int>(oriOrder.status) << oriOrder.id;
    int statusAffected1 = update(updateStatusSql1, statusParams1, errMsg);
    if(statusAffected1 <= 0)
    {
        if(errMsg) *errMsg=*errMsg+" 新订单状态更新失败";
    }

    //差价=新订单价格(创建订单后才有实际价格)-已支付价格
    priceDif=newOrder.priceCents-oriPaidAmount;
    //新订单待支付金额=max(0,差价)  避免被createOrder内部初始化覆盖
    newOrder.pendingPayment=qMax(0,priceDif);

    //更新新订单的pendingPayment(因为createOrder覆盖了 此处需同步到数据库)
    QString updatePendingSql = "update orders set pending_payment=? where id=?";
    QList<QVariant> updateParams;
    updateParams<<newOrder.pendingPayment<<newOrder.id;
    int updateAffected = update(updatePendingSql, updateParams, errMsg);
    if(updateAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+" 新订单待支付金额更新失败";
    }

    //根据pendingPayment->更新新订单状态(大于0->Booked 等于0->Paid)
    qDebug()<<"newOrder.pendingPayment:"<<newOrder.pendingPayment;
    newOrder.status = newOrder.pendingPayment>0 ? (Common::OrderStatus::Booked) : (Common::OrderStatus::Paid);
    QString updateStatusSql2 = "update orders set status=? where id=?";
    QList<QVariant> statusParams2;
    statusParams2 << static_cast<int>(newOrder.status) << newOrder.id;
    int statusAffected2 = update(updateStatusSql2, statusParams2, errMsg);
    if(statusAffected2 <= 0)
    {
        if(errMsg) *errMsg=*errMsg+" 新订单状态更新失败";
    }


    //提交事务
    if(!commitTransaction())
    {
        rollbackTransaction();
        if(errMsg) *errMsg="提交事务失败";
        return DBResult::TransactionFailed;
    }

    return DBResult::Success;
}
//取消订单
DBResult DBManager::cancelOrder(qint64 orderId,bool autoManageTransaction,QString* errMsg)
{
    //开启事务
    if(autoManageTransaction && !beginTransaction())
    {
        if(errMsg) *errMsg=*errMsg+" 开启事务失败";
        return DBResult::TransactionFailed;
    }

    //1.订单状态->取消
    QString orderSql="update orders set status=? , pending_payment=0 where id=? and status not in(?,?)";
    QList<QVariant>orderParams;
    orderParams<<static_cast<int>(Common::OrderStatus::Canceled)<<orderId<<static_cast<int>(Common::OrderStatus::Canceled)<<static_cast<int>(Common::OrderStatus::Finished);    //已取消或已完成的不能再取消

    int orderAffected=update(orderSql,orderParams,errMsg);
    if(orderAffected<=0)
    {
        if(autoManageTransaction) rollbackTransaction();
        if(errMsg) *errMsg=*errMsg+" 订单状态更新失败或订单已取消";
        return DBResult::QueryFailed;
    }

    //2.查询对应的flightId
    QString flightSql="select flight_id from orders where id=?";
    QList<QVariant> flightParams;
    flightParams << orderId;

    //执行查询并校验是否成功
    QSqlQuery flightQuery=Query(flightSql, flightParams, errMsg);
    if(!flightQuery.isActive()) {
        if(autoManageTransaction) rollbackTransaction();
        if (errMsg) *errMsg = *errMsg+" 查询航班ID失败：" + (errMsg->isEmpty() ? "SQL执行错误" : *errMsg);
        return DBResult::QueryFailed;
    }
    //!!!调用next()定位到有效记录
    if (!flightQuery.next()) {
        if(autoManageTransaction) rollbackTransaction();
        if (errMsg) *errMsg = *errMsg+" 未找到订单[" + QString::number(orderId) + "]关联的航班信息";
        return DBResult::NoData;
    }

    qint64 flightId=flightQuery.value("flight_id").toLongLong();
    qDebug()<<"flightId: "<<flightId;
    //3.航班座位+1
    QString seatSql="update flight set seat_left=seat_left+1 where id=?";
    QList<QVariant>seatParams;
    seatParams<<flightId;

    int seatAffected=update(seatSql,seatParams,errMsg);
    if(seatAffected<=0)
    {
        if(autoManageTransaction) rollbackTransaction();
        if(errMsg) *errMsg=*errMsg+" 航班座位恢复失败";
        return DBResult::QueryFailed;
    }

    //4.提交事务
    if(autoManageTransaction && !commitTransaction())
    {
        if(autoManageTransaction) rollbackTransaction();
        if(errMsg) *errMsg=*errMsg+" 提交事务失败";
        return DBResult::TransactionFailed;
    }

    return DBResult::Success;
}


