/*
 * RedisHelper.cpp
 *
 *  Created on: 2016年6月30日
 *      Author: sangechen
 */

#include "RedisHelper.h"

#include <stdexcept> //std::runtime_error
#include <fmt/format.h>
#include <boost/make_shared.hpp>
#include "../../../share/api/ConfigManager.h"
#include "../../../share/lib/OS.h"

namespace redis
{

thread_local bool flag__echo_check_next_time;

boost::shared_ptr<RedisHelper> RedisHelper::getTlsPtr(bool runEchoCheck)
{
    /** 每个线程一份 (thread_local是C++11关键字, __thread是gcc关键字) */
    static thread_local boost::shared_ptr<RedisHelper> tls_ptr;

    //若当前线程的tls_ptr为空, 则new一个实例
    if (!tls_ptr)
    {
        tls_ptr = boost::make_shared<RedisHelper>();
    }

    // 当`flag__echo_check_next_time`被置为true, 或者入参runEchoCheck=true时
    // 执行一次echo命令, 检测当前连接和ctx是否正常. 若异常则新建一个RedisHelper对象
    if (runEchoCheck || flag__echo_check_next_time)
    {
        flag__echo_check_next_time = false;

        // 同步调用echo命令, 参数是当前线程id.
        // 若 抛异常 或 得到的响应结果不等于当前线程id, 则重建连接
        // 这样保证 1.当前redis连接正常 2.req和reply是同步的
        size_t thread_id = OS::getThreadId();
        try
        {
            if (tls_ptr->sendAndRecv(CmdBuilder{"ECHO", thread_id})->getAs<size_t>() != thread_id)
            {
                tls_ptr = boost::make_shared<RedisHelper>();
            }
        }
        catch (...)
        {
            tls_ptr = boost::make_shared<RedisHelper>();
        }
    }

    return tls_ptr;
}

RedisHelper::RedisHelper() : ctx(NULL), lastReplyHolder()
{
    _connect();
}

RedisHelper::~RedisHelper()
{
    _disconnect();
}

void RedisHelper::_connect() //--> exception
{
    config::SocketServer redis_conf = config::ConfigManager::Instance().getServerConfig("redis_server");

    ctx = redisConnect(redis_conf.host.c_str(), redis_conf.port);
    if (ctx == NULL)
    {
        throw std::runtime_error("redisContextInit() error.");
    }
    else if (ctx->err != REDIS_OK)
    {
        throw std::runtime_error(fmt::format("redisConnect() error. [{},{}]", ctx->err, ctx->errstr));
    }
}

void RedisHelper::_reconnect() //--> exception
{
    // TODO 重新获取最新config, 然后再连接??

    int rc = redisReconnect(ctx);
    if (rc != REDIS_OK)
    {
        throw std::runtime_error(fmt::format("redisReconnect() -> {}. [{},{}]", rc, ctx->err, ctx->errstr));
    }
}

void RedisHelper::_disconnect()
{
    redisFree(ctx);
    ctx = NULL;
}

void RedisHelper::sendCmd(const std::vector<std::string>& args)
{
    std::vector<const char*> argv;
    argv.reserve(args.size());
    std::vector<size_t> argvlen;
    argvlen.reserve(args.size());

    for (auto& item : args)
    {
        argv.push_back(item.c_str());
        argvlen.push_back(item.size());
    }

    /// 查看过v0.13.3版源码: 将待发数据追加到write_buf, 然后返回
    /// 只可能出现 `__redisSetError(c,REDIS_ERR_OOM,"Out of memory");`
    int rc = redisAppendCommandArgv(ctx, (int)args.size(), argv.data(), argvlen.data());
    if (rc != REDIS_OK)
    {
        throw std::runtime_error(fmt::format("redisAppendCommandArgv() -> {}. [{},{}]", rc, ctx->err, ctx->errstr));
    }
}

boost::shared_ptr<ReplyHolder> RedisHelper::recvReply()
{
    auto curReplyHolder = boost::make_shared<ReplyHolder>();

    /// 查看过v0.13.3版源码: 会先尝试解析已有buffer, 若无数据再将write_buf发送给redis_server, 然后接受数据到read_buf, 最后再次解析已有buffer.
    /// 可能出现 解析协议错误, 网络I/O错误
    int rc = redisGetReply(ctx, curReplyHolder->getAddr());
    if (rc != REDIS_OK)
    {
        flag__echo_check_next_time = true; //安排下一次请求前做一次`ECHO`, 检测连接和ctx是否依然有效
        throw std::runtime_error(fmt::format("redisGetReply() -> {}. [{},{}]", rc, ctx->err, ctx->errstr));
    }

    lastReplyHolder = curReplyHolder; //记录本次的reply, 析构上一次的ReplyHolder
    return lastReplyHolder;
}

} /* namespace redis */
