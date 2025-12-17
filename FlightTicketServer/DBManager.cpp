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

DBManager& DBManager::instance()   //获取数据库连接对象db
{
    static DBManager inst;
    return inst;
}

//连接数据库
bool DBManager::connect(const QString& host,int port,const QString& user,const QString& passwd,const QString& dbName,QString* errMsg)
{
    if(db.isOpen())
    {
        db.close();
    }

    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(passwd);

    bool status=db.open();
    if(!status && errMsg)
    {
        *errMsg=db.lastError().text();
    }

    return status;
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
    user.phone=query.value("phone").toString();
    user.realName=query.value("real_name").toString();
    user.idCard=query.value("id_card").toString();

    return user;
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
    order.priceCents=query.value("price_cents").toInt();
    order.status=static_cast<Common::OrderStatus>(query.value("status").toInt());

    return order;
}


//封装与业务相关的数据库操作
//用户                    //user作为传出参数
DBResult DBManager::getUserByUserName(const QString& username,Common::UserInfo& user,QString* errMsg)
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

//航班                    //flights作为传出参数
DBResult DBManager::searchFlights(const QString& fromCity,const QString& toCity,const QDate& date,QList<Common::FlightInfo>& flights, QString* errMsg)
{
    QString sql="select * from flight where from_city=? and to_city=? and DATE(depart_time)=? and status=?";

    QList<QVariant>params;          //转换为数据库日期格式
    params<<fromCity<<toCity<<date.toString("yyyy-MM-dd")<<static_cast<int>(Common::FlightStatus::Normal);

    QSqlQuery query=Query(sql,params,errMsg);
    if(!query.isActive()) return DBResult::QueryFailed;

    //遍历结果集
    while(query.next())
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


//订单                    //newOrderId作为传出参数
DBResult DBManager::createOrder(const Common::OrderInfo& order,qint64& newOrderId,QString* errMsg)
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

    //2.插入订单数据
    QString orderSql="insert into orders (user_id,flight_id,passenger_name,passenger_id_card,price_cents,status) values(?,?,?,?,?,?)";
    QList<QVariant>orderParams;
    orderParams<<order.userId<<order.flightId<<order.passengerName<<order.passengerIdCard<<order.priceCents<<static_cast<int>(order.status);

    QSqlQuery orderQuery=Query(orderSql,orderParams,errMsg);
    if(!orderQuery.isActive())
    {
        rollbackTransaction();
        return DBResult::QueryFailed;
    }
    //获取新订单ID
    newOrderId=orderQuery.lastInsertId().toLongLong();

    //提交事务/中途事务回滚
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
    while(query.next())
    {
        orders.append(orderFromQuery(query));
    }

    return orders.isEmpty()? DBResult::NoData : DBResult::Success;
}
DBResult DBManager::cancelOrder(qint64 orderId,qint64 flightId,QString* errMsg)
{
    //开启事务
    if(!beginTransaction())
    {
        if(errMsg) *errMsg="开启事务失败";
        return DBResult::TransactionFailed;
    }

    //1.订单状态->取消
    QString orderSql="update orders set status=? where id=? and status!=?";
    QList<QVariant>orderParams;
    orderParams<<static_cast<int>(Common::OrderStatus::Canceled)<<orderId<<static_cast<int>(Common::OrderStatus::Canceled);    //排除已消除的订单

    int orderAffected=update(orderSql,orderParams,errMsg);
    if(orderAffected<=0)
    {
        rollbackTransaction();
        if(errMsg) *errMsg="订单状态更新失败或订单已取消";
        return DBResult::QueryFailed;
    }

    //2.航班座位+1
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

    //提交事务
    if(!commitTransaction())
    {
        rollbackTransaction();
        if(errMsg) *errMsg="提交事务失败";
        return DBResult::TransactionFailed;
    }

    return DBResult::Success;
}


