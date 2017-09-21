/*
 * reply_parse_func.hpp
 *
 *  Created on: 2016年6月30日
 *      Author: sangechen
 */

#ifndef SHARE_FW_REDIS_REPLY_PARSE_FUNC_HPP_
#define SHARE_FW_REDIS_REPLY_PARSE_FUNC_HPP_

#include <map>
#include <stdexcept> //std::runtime_error
#include <vector>
#include <boost/lexical_cast.hpp>
#include <hiredis/hiredis.h>

namespace redis
{

/**
 * 这里定义了多个重复的函数模板,
 * 编译时会根据实参类型 选出最优的一份进行展开编译.
 */

/**
 * 函数返回false表示REDIS_REPLY_NIL
 * 函数抛std::runtime_error异常表示REDIS_REPLY_类型不匹配
 * 函数抛boost::bad_cast异常表示单个值在做类型转换时报错: 例如将1.23(float)转成int
 */

/**
 * ret=lexical_cast<T>(`value`)
 *
 * return (reply->type != Null)
 */
template <typename Result_T> //若命中此模板, 则说明T是单一类型
static bool _parseXX(redisReply* reply, Result_T& ret)
{
    if (reply->type == REDIS_REPLY_NIL)
    {
        return false;
    }
    else if (reply->type == REDIS_REPLY_INTEGER)
    {
        ret = boost::lexical_cast<Result_T>(reply->integer);
        return true;
    }
    else if (reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR)
    {
        ret = boost::lexical_cast<Result_T>(reply->str, reply->len);
        return true;
    }
    else // REDIS_REPLY_ARRAY
    {
        throw std::runtime_error("_parseOne() cannot process REDIS_REPLY_ARRAY");
    }
}

/**
 * ret.clear()
 * ret.insert(ret.end(), `value`)
 *
 * return (reply->type != Null)
 */
template <typename T>
static bool _parseXX(redisReply* reply, std::vector<T>& vecRet)
{
    if (reply->type == REDIS_REPLY_NIL)
    {
        return false;
    }
    else if (reply->type == REDIS_REPLY_ARRAY)
    {
        vecRet.resize(reply->elements);
        for (size_t i = 0; i < reply->elements; i++)
        {
            // todo 若有Null元素, 则取T类型的默认值
            _parseXX(reply->element[i], vecRet[i]);
        }

        return true;
    }
    else
    {
        throw std::runtime_error("_parseArray() cannot process !REDIS_REPLY_ARRAY");
    }
}

/**
 * ret.clear()
 * ret[`key`] = `value`
 *
 * return (reply->type != Null)
 */
template <typename K, typename V>
static bool _parseXX(redisReply* reply, std::map<K, V>& mapRet)
{
    if (reply->type == REDIS_REPLY_NIL)
    {
        return false;
    }
    else if (reply->type == REDIS_REPLY_ARRAY)
    {
        std::pair<K, V> pair;

        mapRet.clear();
        for (size_t i = 0; (i + 1) < reply->elements; i += 2)
        {
            // todo 若有Null元素, 则取K/V类型的默认值
            _parseXX(reply->element[i], pair.first);
            _parseXX(reply->element[i + 1], pair.second);

            mapRet.insert(pair);
        }

        return true;
    }
    else
    {
        throw std::runtime_error("_parseMap() cannot process !REDIS_REPLY_ARRAY");
    }
}

} /* namespace redis */

#endif /* SHARE_FW_REDIS_REPLY_PARSE_FUNC_HPP_ */
