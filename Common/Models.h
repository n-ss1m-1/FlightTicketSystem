#ifndef MODELS_H
#define MODELS_H

// ============================================
// Common/Models.h
// 客户端与服务端共用的数据模型 + JSON 转换工具
//
// 统一客户端与服务端的数据结构定义，避免两边字段不一致；
// 提供结构体与 QJsonObject/QJsonArray 的互转函数，方便网络传输与解析；
// 提供基础数据合法性校验函数（isValidFlight/isValidUser）。
//
// 金额统一使用“分”（int）存储与传输，例如 12345 表示 123.45 元；
// 时间统一使用 ISO 字符串（Qt::ISODate）传输，例如 "2025-12-14T10:30:00"；
// ID 统一使用 64 位整数（qint64），对应 MySQL BIGINT；
// 状态字段使用 int 枚举值。
//
// 注意：
// - 客户端：在接收服务端数据并解析/展示航班、订单、用户信息的文件中需要 #include "Common/Models.h"；
// - 服务端：在从数据库读取并封装数据、以及将数据转为 JSON 返回给客户端的文件中需要 #include "Common/Models.h"。
// ============================================


#include <QtGlobal>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace Common {

// ======================== 枚举定义 ========================

enum class FlightStatus : qint32 {
    Normal   = 0,   // 正常
    Canceled = 1,   // 取消
    Delayed  = 2    // 延误
};

enum class OrderStatus : qint32 {
    Booked   = 0,   // 已预订
    Canceled = 1,   // 已取消/退票
    Finished = 2    // 已完成
};

// ======================== 通用工具函数 ========================

// QDateTime -> ISO 字符串（网络传输用）
inline QString toIsoString(const QDateTime &dt)
{
    return dt.toString(Qt::ISODate);
}

// ISO 字符串 -> QDateTime（解析失败会得到 invalid 的 QDateTime）
inline QDateTime fromIsoString(const QString &s)
{
    return QDateTime::fromString(s, Qt::ISODate);
}

// 如果字符串非空才写入 JSON（可减少无意义字段）
inline void putIfNotEmpty(QJsonObject &o, const char *key, const QString &v)
{
    if (!v.isEmpty()) o.insert(key, v);
}

// ======================== 数据模型 ========================

// 航班信息
struct FlightInfo {
    qint64 id = 0;                 // 航班记录ID（数据库主键）
    QString flightNo;              // 航班号
    QString fromCity;              // 出发城市
    QString toCity;                // 到达城市
    QDateTime departTime;          // 起飞时间
    QDateTime arriveTime;          // 到达时间

    qint32 priceCents = 0;         // 票价（分）
    qint32 seatTotal = 0;          // 总座位数
    qint32 seatLeft = 0;           // 剩余座位数
    FlightStatus status = FlightStatus::Normal; // 航班状态
};

// 用户信息
struct UserInfo {
    qint64 id = 0;                 // 用户ID（数据库主键）
    QString username;              // 登录用户名
    QString password;              // 登陆密码
    QString phone;                 // 手机号
    QString realName;              // 真实姓名
    QString idCard;                // 身份证号
};

// 常用乘机人信息
struct PassengerInfo {
    qint64 id = 0;                  // 常用乘机人ID 主键
    qint64 user_id = 0;             //所属用户ID
    QString name;                   // 姓名
    QString idCard;                 // 身份证号
};

// 订单信息
struct OrderInfo {
    qint64 id = 0;                 // 订单ID（数据库主键）

    qint64 userId = 0;             // 下单用户ID
    qint64 flightId = 0;           // 航班ID

    // 乘机人信息
    QString passengerName;
    QString passengerIdCard;

    QString seatNum;               // 座位号
    qint32 priceCents = 0;         // 成交价格（分）
    OrderStatus status = OrderStatus::Booked; // 订单状态
    QDateTime createdTime;         // 下单时间
};

// ======================== JSON 序列化/反序列化 ========================

// -------- 航班 FlightInfo <-> JSON --------
inline QJsonObject flightToJson(const FlightInfo &f)
{
    QJsonObject o;
    o.insert("id", static_cast<qint64>(f.id));
    o.insert("flightNo", f.flightNo);
    o.insert("fromCity", f.fromCity);
    o.insert("toCity", f.toCity);
    o.insert("departTime", toIsoString(f.departTime));
    o.insert("arriveTime", toIsoString(f.arriveTime));
    o.insert("priceCents", f.priceCents);
    o.insert("seatTotal", f.seatTotal);
    o.insert("seatLeft", f.seatLeft);
    o.insert("status", static_cast<qint32>(f.status));
    return o;
}

inline FlightInfo flightFromJson(const QJsonObject &o)
{
    FlightInfo f;
    f.id = static_cast<qint64>(o.value("id").toVariant().toLongLong());
    f.flightNo = o.value("flightNo").toString();
    f.fromCity = o.value("fromCity").toString();
    f.toCity = o.value("toCity").toString();
    f.departTime = fromIsoString(o.value("departTime").toString());
    f.arriveTime = fromIsoString(o.value("arriveTime").toString());
    f.priceCents = o.value("priceCents").toInt();
    f.seatTotal = o.value("seatTotal").toInt();
    f.seatLeft = o.value("seatLeft").toInt();
    f.status = static_cast<FlightStatus>(o.value("status").toInt());
    return f;
}

inline QJsonArray flightsToJsonArray(const QList<FlightInfo> &list)
{
    QJsonArray arr;
    for (const auto &f : list) arr.append(flightToJson(f));
    return arr;
}

inline QList<FlightInfo> flightsFromJsonArray(const QJsonArray &arr)
{
    QList<FlightInfo> list;
    list.reserve(arr.size());
    for (const auto &v : arr) {
        if (v.isObject()) list.push_back(flightFromJson(v.toObject()));
    }
    return list;
}

// -------- 用户 UserInfo <-> JSON --------
inline QJsonObject userToJson(const UserInfo &u)
{
    QJsonObject o;
    o.insert("id", static_cast<qint64>(u.id));
    o.insert("username", u.username);
    putIfNotEmpty(o, "phone", u.phone);
    putIfNotEmpty(o, "realName", u.realName);
    putIfNotEmpty(o, "idCard", u.idCard);
    return o;
}

inline UserInfo userFromJson(const QJsonObject &o)
{
    UserInfo u;
    u.id = static_cast<qint64>(o.value("id").toVariant().toLongLong());
    u.username = o.value("username").toString();
    u.phone = o.value("phone").toString();
    u.realName = o.value("realName").toString();
    u.idCard = o.value("idCard").toString();
    return u;
}

// -------- 常用乘机人 PassengerInfo <-> JSON --------
inline QJsonObject passengerToJson(const PassengerInfo &ord)
{
    QJsonObject o;
    o.insert("id", static_cast<qint64>(ord.id));
    o.insert("userId", static_cast<qint64>(ord.user_id));
    putIfNotEmpty(o, "name", ord.name);
    putIfNotEmpty(o, "idCard", ord.idCard);
    return o;
}

inline PassengerInfo passengerFromJson(const QJsonObject &o)
{
    PassengerInfo ord;
    ord.id = static_cast<qint64>(o.value("id").toVariant().toLongLong());
    ord.user_id = static_cast<qint64>(o.value("userId").toVariant().toLongLong());
    ord.name = o.value("name").toString();
    ord.idCard = o.value("idCard").toString();

    return ord;
}

inline QJsonArray passengersToJsonArray(const QList<PassengerInfo> &list)
{
    QJsonArray arr;
    for (const auto &o : list) arr.append(passengerToJson(o));
    return arr;
}

inline QList<PassengerInfo> passengersFromJsonArray(const QJsonArray &arr)
{
    QList<PassengerInfo> list;
    list.reserve(arr.size());
    for (const auto &v : arr) {
        if (v.isObject()) list.push_back(passengerFromJson(v.toObject()));
    }
    return list;
}

// -------- 订单 OrderInfo <-> JSON --------
inline QJsonObject orderToJson(const OrderInfo &ord)
{
    QJsonObject o;
    o.insert("id", static_cast<qint64>(ord.id));
    o.insert("userId", static_cast<qint64>(ord.userId));
    o.insert("flightId", static_cast<qint64>(ord.flightId));

    putIfNotEmpty(o, "passengerName", ord.passengerName);
    putIfNotEmpty(o, "passengerIdCard", ord.passengerIdCard);

    putIfNotEmpty(o, "seatNum", ord.seatNum);
    o.insert("priceCents", ord.priceCents);
    o.insert("status", static_cast<qint32>(ord.status));
    o.insert("createdTime", toIsoString(ord.createdTime));
    return o;
}

inline OrderInfo orderFromJson(const QJsonObject &o)
{
    OrderInfo ord;
    ord.id = static_cast<qint64>(o.value("id").toVariant().toLongLong());
    ord.userId = static_cast<qint64>(o.value("userId").toVariant().toLongLong());
    ord.flightId = static_cast<qint64>(o.value("flightId").toVariant().toLongLong());

    ord.passengerName = o.value("passengerName").toString();
    ord.passengerIdCard = o.value("passengerIdCard").toString();

    ord.seatNum = o.value("seatNum").toString();
    ord.priceCents = o.value("priceCents").toInt();
    ord.status = static_cast<OrderStatus>(o.value("status").toInt());
    ord.createdTime = fromIsoString(o.value("createdTime").toString());
    return ord;
}

inline QJsonArray ordersToJsonArray(const QList<OrderInfo> &list)
{
    QJsonArray arr;
    for (const auto &o : list) arr.append(orderToJson(o));
    return arr;
}

inline QList<OrderInfo> ordersFromJsonArray(const QJsonArray &arr)
{
    QList<OrderInfo> list;
    list.reserve(arr.size());
    for (const auto &v : arr) {
        if (v.isObject()) list.push_back(orderFromJson(v.toObject()));
    }
    return list;
}

// ======================== 基础校验 ========================

inline bool isValidFlight(const FlightInfo &f, QString *err = nullptr)
{
    if (f.flightNo.isEmpty()) { if (err) *err = "flightNo 为空"; return false; }
    if (f.fromCity.isEmpty() || f.toCity.isEmpty()) { if (err) *err = "城市信息为空"; return false; }
    if (!f.departTime.isValid() || !f.arriveTime.isValid()) { if (err) *err = "时间无效"; return false; }
    if (f.priceCents < 0) { if (err) *err = "价格不能为负数"; return false; }
    if (f.seatTotal < 0 || f.seatLeft < 0 || f.seatLeft > f.seatTotal) {
        if (err) *err = "座位数不合法";
        return false;
    }
    return true;
}

inline bool isValidUser(const UserInfo &u, QString *err = nullptr)
{
    if (u.username.isEmpty()) { if (err) *err = "username 为空"; return false; }
    return true;
}

}

#endif
