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
Common::UserInfo DBManager::userFromQuery(const QSqlQuery& query)
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
Common::PassengerInfo DBManager::passengerFromQuery(const QSqlQuery& query)
{
    Common::PassengerInfo passenger;
    passenger.id=query.value("id").toLongLong();
    passenger.name=query.value("name").toString();
    passenger.idCard=query.value("idCard").toString();
    return passenger;
}
Common::FlightInfo DBManager::flightFromQuery(const QSqlQuery& query)
{
    Common::FlightInfo flight;
    flight.id=query.value("id").toLongLong();
    flight.flightNo=query.value("flight_no").toString();
    flight.fromCity=query.value("from_city").toString();
    flight.toCity=query.value("to_city").toString();
    flight.departTime=query.value("depart_time").toDateTime();
    flight.arriveTime=query.value("arrive_time").toDateTime();
    flight.priceCents=query.value("price_cents").toInt();
    flight.seatTotal=query.value("seat_total").toInt();
    flight.seatLeft=query.value("seat_left").toInt();
    flight.status=static_cast<Common::FlightStatus>(query.value("status").toInt());

    return flight;
}
Common::OrderInfo DBManager::orderFromQuery(const QSqlQuery& query)
{
    Common::OrderInfo order;
    order.id=query.value("id").toLongLong();
    order.userId=query.value("user_id").toLongLong();
    order.flightId=query.value("flight_id").toLongLong();
    order.passengerName=query.value("passenger_name").toString();
    order.passengerIdCard=query.value("passenger_id_card").toString();
    order.seatNum=query.value("seat_num").toString();
    order.priceCents=query.value("price_cents").toInt();
    order.status=static_cast<Common::OrderStatus>(query.value("status").toInt());
    
    order.createdTime = query.value("created_time").toDateTime();


    order.createdTime = query.value("created_time").toDateTime();

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
DBResult DBManager::addPassenger(const qint64 user_id,const QString& passenger_name,const QString& passenger_id_card,QString* errMsg)
{
    //1.插入乘机人表
    QString passengerSql="insert into passenger (name,id_card) values(?,?)";
    QList<QVariant> passengerParams;
    passengerParams<<passenger_name<<passenger_id_card;

    QSqlQuery passengerQuery(db);
    if(!passengerQuery.prepare(passengerSql))
    {
        if(errMsg) *errMsg="SQL prepare failed: "+passengerQuery.lastError().text();
        qWarning()<<"SQL prepare failed: "<<passengerSql<<"Error:"<<passengerQuery.lastError().text();
    }

    for(int i=0; i<passengerParams.size();i++)
    {
        passengerQuery.bindValue(i, passengerParams[i]);  //位置绑定
    }

    if(!passengerQuery.exec())
    {
        if(errMsg)
        {
            *errMsg="sql exec failed: "+passengerQuery.lastError().text();
            qWarning()<<"sql exec failed: "<<passengerSql<<"Error:"<<passengerQuery.lastError().text();
        }
    }
    if(passengerQuery.numRowsAffected()<=0)
    {
        if(errMsg) *errMsg=*errMsg+"电话号码更改失败";
        return DBResult::updateFailed;
    }

    //2.查找乘机人ID
    QVariant passenger_id = passengerQuery.lastInsertId();

    //3.插入user_passenger关联表
    QString U_P_sql="insert into user_passenger (user_id,passenger_id) values(?,?)";
    QList<QVariant> U_P_params;
    U_P_params<<user_id<<passenger_id;

    int sqlAffected=update(U_P_sql,U_P_params,errMsg);
    if(sqlAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+"加入乘机人失败";
        return DBResult::updateFailed;
    }

    return DBResult::Success;
}
DBResult  DBManager::delPassenger(const QString& passenger_name,const QString& passenger_id_card,QString* errMsg)
{
    QString sql="delete from passenger where name=? and id_card=?";
    QList<QVariant> params;
    params<<passenger_name<<passenger_id_card;

    int sqlAffected=update(sql,params,errMsg);
    if(sqlAffected<=0)
    {
        if(errMsg) *errMsg=*errMsg+"删除乘机人失败";
        return DBResult::updateFailed;
    }

    return DBResult::Success;
}
DBResult DBManager::getPassengers(const qint64 user_id,QList<Common::PassengerInfo>& passengers,QString* errMsg)
{
    QString sql="select p.* "
                  "from user_passenger up "
                  "inner join user u on up.user_id=u.id "
                  "inner join passenger p on up.passenger_id=p.id "
                  "where u.id=user_id";
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

//航班                    //flights作为传出参数
DBResult DBManager::searchFlights(const QString& fromCity,const QString& toCity,const QDate& date,QList<Common::FlightInfo>& flights,QString* errMsg)
{
    QString sql="select * from flight where from_city=? and to_city=? and DATE(depart_time)=? and status=? order by depart_time asc";    //此处使用sql函数 取出日期

    QList<QVariant>params;          //转换为数据库date格式->匹配
    params<<fromCity<<toCity<<date.toString("yyyy-MM-dd")<<static_cast<int>(Common::FlightStatus::Normal);

    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive()) return DBResult::QueryFailed;

    //遍历结果集
    while(query.next())     //初始位置：-1
    {
        flights.append(flightFromQuery(query));
    }

    return flights.isEmpty()?DBResult::NoData : DBResult::Success;
}
DBResult DBManager::getFlightById(qint64 flightId,Common::FlightInfo& flight,QString* errMsg)
{
    QString sql="select * from flight where id=?";
    QList<QVariant>params;
    params<<flightId;

    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive()) return DBResult::QueryFailed;
    if(!query.next()) return DBResult::NoData;

    flight=flightFromQuery(query);
    return DBResult::Success;
}


//订单
DBResult DBManager::createOrder(Common::OrderInfo& order,QString* errMsg)
{
    //开启事务
    if(!beginTransaction())
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
        rollbackTransaction();
        if(errMsg)
        {
            QString detail=!(*errMsg).isEmpty()?*errMsg:"无可用座位";
            *errMsg="航班座位不足或更新失败: "+detail;
        }
        return DBResult::QueryFailed;
    }

    //2.更新订单座位
    Common::FlightInfo flight;
    if(getFlightById(order.flightId, flight, errMsg)!=DBResult::Success)
    {
        rollbackTransaction();
        if (errMsg) *errMsg = "获取航班信息失败: " + *errMsg;
        return DBResult::QueryFailed;
    }
    order.seatNum=QString::number(flight.seatTotal-flight.seatLeft+1);

    //3.通过flightId查询订单价格
    QString priceSql="select price_cents from flight where id=?";
    QList<QVariant> priceParams;
    priceParams<<order.flightId;
    QSqlQuery priceQuery=Query(priceSql, priceParams, errMsg);
    if(!priceQuery.isActive() || !priceQuery.next())
    {
        rollbackTransaction();
        if (errMsg) *errMsg="查询航班价格失败: 无该航班价格信息";
        return DBResult::QueryFailed;
    }
    order.priceCents=priceQuery.value("price_cents").toInt();

    //4.初始化订单状态：0 Booked
    order.status=Common::OrderStatus::Booked;

    //5.插入订单数据
    QString orderSql="insert into orders (user_id,flight_id,passenger_name,passenger_id_card,seat_num,price_cents,status) values(?,?,?,?,?,?,?)";
    QList<QVariant>orderParams;
    orderParams<<order.userId<<order.flightId<<order.passengerName<<order.passengerIdCard<<order.seatNum<<order.priceCents<<static_cast<int>(order.status);

    QSqlQuery orderQuery=Query(orderSql,orderParams,errMsg);
    if(!orderQuery.isActive())
    {
        rollbackTransaction();
        return DBResult::QueryFailed;
    }
    //6.获取新订单ID
    order.id=orderQuery.lastInsertId().toLongLong();

    //7.提交事务/中途事务回滚
    if(!commitTransaction())
    {
        rollbackTransaction();
        if(errMsg) *errMsg="提交事务失败";
        return DBResult::TransactionFailed;
    }
    return DBResult::Success;
}
DBResult DBManager::getOrdersByUserId(qint64 userId,QList<Common::OrderInfo>& orders,QString* errMsg)
{
    //sql语句和参数
    QString sql="select * from orders where user_id=? order by id desc";
    QList<QVariant>params;
    params<<userId;

    //执行sql
    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive())
    {
        return DBResult::QueryFailed;
    }

    //遍历结果集
    orders.clear();
    while(query.next())     //初始位置：-1
    {
        orders.append(orderFromQuery(query));
    }

    return orders.isEmpty()? DBResult::NoData : DBResult::Success;
}
DBResult DBManager::cancelOrder(qint64 userId,qint64 orderId,QString* errMsg)
{
    //开启事务
    if(!beginTransaction())
    {
        if(errMsg) *errMsg="开启事务失败";
        return DBResult::TransactionFailed;
    }

    //1.订单状态->取消
    QString orderSql="update orders set status=? where user_id=? and id=? and status=?";
    QList<QVariant>orderParams;
    orderParams<<static_cast<int>(Common::OrderStatus::Canceled)<<userId<<orderId<<static_cast<int>(Common::OrderStatus::Booked);    //预定的可以取消，已取消或已完成的不能再取消

    int orderAffected=update(orderSql,orderParams,errMsg);
    if(orderAffected<=0)
    {
        rollbackTransaction();
        if(errMsg) *errMsg="订单状态更新失败或订单已取消";
        return DBResult::QueryFailed;
    }

    //2.查询对应的flightId
    QString flightSql="select flight_id from orders where id=?";
    QList<QVariant> flightParams;
    flightParams << orderId;

    //执行查询并校验是否成功
    QSqlQuery flightQuery=Query(flightSql, flightParams, errMsg);
    if(!flightQuery.isActive()) {
        rollbackTransaction();
        if (errMsg) *errMsg = "查询航班ID失败：" + (errMsg->isEmpty() ? "SQL执行错误" : *errMsg);
        return DBResult::QueryFailed;
    }
    //!!!调用next()定位到有效记录
    if (!flightQuery.next()) {
        rollbackTransaction();
        if (errMsg) *errMsg = "未找到订单[" + QString::number(orderId) + "]关联的航班信息";
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
        rollbackTransaction();
        if(errMsg) *errMsg="航班座位恢复失败";
        return DBResult::QueryFailed;
    }

    //4.提交事务
    if(!commitTransaction())
    {
        rollbackTransaction();
        if(errMsg) *errMsg="提交事务失败";
        return DBResult::TransactionFailed;
    }

    return DBResult::Success;
}


