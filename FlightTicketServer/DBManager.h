#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QList>
#include <QVariant>     //类型转换
#include <QDateTime>    //时间类型
#include "Common/Models.h"  //引入数据类型

//操作结果的状态
enum class DBResult
{
    Success,
    ConnectFailed,
    QueryFailed,
    NoData,
    TransactionFailed,
    updateFailed
};

class DBManager
{
public:
    /*
     * 单例模式
     * 避免资源浪费
     * 保证连接状态一致
     * 简化调用逻辑(统一使用instance获取该对象)
    */
    static DBManager& instance();  //获取数据库连接对象
    DBManager(const DBManager&)=delete;
    DBManager& operator=(const DBManager&)=delete;

    //errMsg作为传出参数：给 调用者/用户 提示信息
    //连接数据库
    bool connect(const QString& host,int port,const QString& user,const QString& passwd,const QString& dbName,QString* errMsg=nullptr);

    bool isConnected() const;

    //查询操作
    QSqlQuery Query(const QString& sql,const QList<QVariant>& params = QList<QVariant>(),QString* errMsg=nullptr);

    //增删改操作  返回受影响的行数
    int update(const QString& sql,const QList<QVariant>& params = QList<QVariant>(),QString* errMsg=nullptr);

    //事务操作
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    //封装与业务相关的数据库操作
    //用户                    //user作为传出参数
    DBResult getUserByUsername(const QString& username,Common::UserInfo& user,QString* errMsg=nullptr);
    DBResult getUserById(qint64 userId,Common::UserInfo& user,QString* errMsg=nullptr);
    DBResult addUser(const QString& username,const QString& password,const QString& phone,const QString& real_name,const QString& id_card,QString* errMsg);
    DBResult updatePasswdByUsername(const QString& username,const QString& newPasswd,QString* errMsg);
    DBResult updatePhoneByUsername(const QString& username,const QString& phone,QString* errMsg);
    DBResult addPassenger(const qint64 user_id,const QString& passenger_name,const QString& passenger_id_card,QString* errMsg);
    DBResult getPassengers(const qint64 user_id,QList<Common::PassengerInfo>& passengers,QString* errMsg);
    //航班                    //flights作为传出参数
    DBResult searchFlights(const QString& fromCity,const QString& toCity,const QDate& date,QList<Common::FlightInfo>& flights, QString* errMsg=nullptr);
    DBResult getFlightById(qint64 flightId,Common::FlightInfo& flight,QString* errMsg=nullptr);

    //订单                    //order作为传出参数
    DBResult createOrder(Common::OrderInfo& order,QString* errMsg=nullptr);
    DBResult getOrdersByUserId(qint64 userId,QList<Common::OrderInfo>& orders,QString* errMsg=nullptr);
    DBResult cancelOrder(qint64 userId,qint64 orderId,QString* errMsg=nullptr);       //通过userId和orderId来定位,避免用户A取消用户B的订单

private:
    DBManager();       //单例模式
    ~DBManager();

    //数据库连接对象
    QSqlDatabase db;

    //将查询结果转换为相应的Info
    Common::UserInfo userFromQuery(const QSqlQuery& query);
    Common::FlightInfo flightFromQuery(const QSqlQuery& query);
    Common::OrderInfo orderFromQuery(const QSqlQuery& query);
    Common::PassengerInfo passengerFromQuery(const QSqlQuery& query);
};


#endif // DBMANAGER_H
