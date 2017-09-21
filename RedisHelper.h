/*
 * RedisHelper.h
 *
 *  Created on: 2016年6月30日
 *      Author: sangechen
 */

#ifndef SHARE_FW_REDIS_REDISHELPER_H_
#define SHARE_FW_REDIS_REDISHELPER_H_

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <hiredis/hiredis.h> //using `Synchronous API`

#include "CmdBuilder.h"
#include "ReplyHolder.h"

namespace redis
{

extern thread_local bool flag__echo_check_next_time;

class RedisHelper : private boost::noncopyable
{
public:
    /**
     * 使用`redis::RedisHelper::getTlsPtr(true);` 实现立即重连
     * 使用`redis::flag__echo_check_next_time = true;` 实现lazy重连
     */
    static boost::shared_ptr<RedisHelper> getTlsPtr(bool runEchoCheck = false);

public:
    RedisHelper();
    virtual ~RedisHelper();

protected:
    /** redisContext *redisConnect(const char *ip, int port); */
    void _connect(); //--> exception

    /** int redisReconnect(redisContext *c); */
    void _reconnect(); //--> exception

    /** void redisFree(redisContext *c); */
    void _disconnect();

public:
    /** int redisAppendCommandArgv(redisContext *c, int argc, const char **argv, const size_t *argvlen); */
    void sendCmd(const std::vector<std::string>& args);

    /**
     * int redisGetReply(redisContext *c, void **reply);
     * void freeReplyObject(void *reply);
     */
    boost::shared_ptr<ReplyHolder> recvReply();
    inline boost::shared_ptr<ReplyHolder> getLastReply()
    {
        return lastReplyHolder;
    }

    inline boost::shared_ptr<ReplyHolder> sendAndRecv(const std::vector<std::string>& args)
    {
        sendCmd(args);
        return recvReply();
    }

protected:
    redisContext* ctx;

    /** 用于记录下redis服务端的回复 */
    // std::list<boost::shared_ptr<ReplyHolder>> replyList;
    boost::shared_ptr<ReplyHolder> lastReplyHolder;
};

} /* namespace redis */

#endif /* SHARE_FW_REDIS_REDISHELPER_H_ */
