#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QJsonObject>

// ============================================
// Common/Protocol.h
// 客户端与服务端共用的“通信协议定义”文件：统一管理 JSON 通信所需的 key、type 常量，
// 并提供统一的响应构造函数（makeOkResponse / makeFailResponse），避免在各处手写字符串导致不一致。
//
// 所有消息均为 JSON 文本，通过 TCP 发送,每条消息以 '\n' 结尾用于分隔/拆包。
// JSON 顶层字段统一使用：type / data / success / message（可选 reqId）。
// 请求：{ "type": xxx, "data": {...} }；响应：{ "type": xxx_response, "success": bool, "message": "...", "data": {...} }。
//
// 请在客户端与服务端的所有网络收发/解析代码中 #include "Common/Protocol.h" 使用这些常量与函数，
// 不要在其他文件里直接写 "login"、"login_response" 等字符串。
// ============================================


namespace Protocol {

// ======================== JSON 顶层字段名 ========================

static const char* KEY_TYPE    = "type";
static const char* KEY_DATA    = "data";
static const char* KEY_SUCCESS = "success";
static const char* KEY_MESSAGE = "message";
static const char* KEY_REQID   = "reqId";   // 可选：并发请求时使用

// ======================== 通用 type ========================

static const QString TYPE_ERROR = "error";

// ======================== 登录 ========================

static const QString TYPE_LOGIN      = "login";
static const QString TYPE_LOGIN_RESP = "login_response";

// ======================== 用户管理  ========================
static const QString TYPE_REGISTER      = "register";
static const QString TYPE_REGISTER_RESP = "register_response";
static const QString TYPE_CHANGE_PWD    = "change_pwd";
static const QString TYPE_CHANGE_PWD_RESP = "change_pwd_response";
static const QString TYPE_UPDATE_INFO   = "update_info";
static const QString TYPE_UPDATE_INFO_RESP = "update_info_response";

// ======================== 航班 ========================

static const QString TYPE_FLIGHT_SEARCH      = "flight_search";
static const QString TYPE_FLIGHT_SEARCH_RESP = "flight_search_response";

// ======================== 订单 ========================

// 下单
static const QString TYPE_ORDER_CREATE      = "order_create";
static const QString TYPE_ORDER_CREATE_RESP = "order_create_response";

// 订单列表
static const QString TYPE_ORDER_LIST      = "order_list";
static const QString TYPE_ORDER_LIST_RESP = "order_list_response";

// 取消订单 / 退票
static const QString TYPE_ORDER_CANCEL      = "order_cancel";
static const QString TYPE_ORDER_CANCEL_RESP = "order_cancel_response";

// ======================== 响应构造工具 ========================

// 成功响应
inline QJsonObject makeOkResponse(
    const QString &respType,
    const QJsonObject &data = QJsonObject(),
    const QString &message = "ok")
{
    QJsonObject obj;
    obj.insert(KEY_TYPE, respType);
    obj.insert(KEY_SUCCESS, true);
    obj.insert(KEY_MESSAGE, message);
    obj.insert(KEY_DATA, data);
    return obj;
}

// 失败响应
inline QJsonObject makeFailResponse(
    const QString &respType,
    const QString &message,
    const QJsonObject &data = QJsonObject())
{
    QJsonObject obj;
    obj.insert(KEY_TYPE, respType);
    obj.insert(KEY_SUCCESS, false);
    obj.insert(KEY_MESSAGE, message);
    obj.insert(KEY_DATA, data);
    return obj;
}

} // namespace Protocol

#endif // PROTOCOL_H
