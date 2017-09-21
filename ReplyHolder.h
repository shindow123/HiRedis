/*
 * ReplyHolder.h
 *
 *  Created on: 2016年6月30日
 *      Author: sangechen
 */

#ifndef SHARE_FW_REDIS_REPLYHOLDER_H_
#define SHARE_FW_REDIS_REPLYHOLDER_H_

#include <stdexcept> //std::runtime_error
#include <boost/noncopyable.hpp>
#include <hiredis/hiredis.h>

#include "reply_parse_func.hpp"

namespace redis
{

/**
 * ReplyHolder持有并管理redisGetReply()返回的一个指针: 在析构时会自动调用freeReplyObject()释放该指针.
 * ReplyHolder不能copy, 避免多个对象析构时多次重复调用freeReplyObject()
 * ReplyHolder的parse函数即使抛异常也不会影响RedisHelper对象.
 */
class ReplyHolder : private boost::noncopyable
{
public:
    ReplyHolder();
    virtual ~ReplyHolder();

    /** pass to redisGetReply() */
    void** getAddr();

    /** read-only: parse reply data */
    const redisReply* ptr();

public:
    template <typename XX>
    bool parseTo(XX& result)
    {
        return _parseXX(reply_ptr, result);
    }

    template <typename XX>
    bool parseScanInto(uint64_t& cursor, XX& result)
    {
        if (reply_ptr->type != REDIS_REPLY_ARRAY || reply_ptr->elements < 2)
        {
            return false;
        }

        _parseXX(reply_ptr->element[0], cursor);
        _parseXX(reply_ptr->element[1], result);

        return true;
    }

    /** 直接返回结果. 如果类型不匹配,抛异常 */
    template <typename XX>
    XX getAs()
    {
        XX ret;
        ///{ret若不修改则一定会抛异常, 所以ret的值理论上不会存在随机性}
        // if (std::is_arithmetic<XX>) //int或double类型的栈变量 初始值可能随机
        //{
        //	ret = 0;
        //}

        if (!parseTo(ret))
        {
            throw std::runtime_error("REDIS_REPLY_NIL");
        }

        return ret;
    }

private:
    redisReply* reply_ptr;
};

} /* namespace redis */

#endif /* SHARE_FW_REDIS_REPLYHOLDER_H_ */
