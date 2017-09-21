/*
 * ReplyHolder.cpp
 *
 *  Created on: 2016年6月30日
 *      Author: sangechen
 */

#include "ReplyHolder.h"

namespace redis
{

ReplyHolder::ReplyHolder()
{
    reply_ptr = NULL;
}

ReplyHolder::~ReplyHolder()
{
    if (reply_ptr != NULL)
    {
        freeReplyObject((void*)reply_ptr);
        reply_ptr = NULL;
    }
}

void** ReplyHolder::getAddr()
{
    return (void**)&reply_ptr;
}

const redisReply* ReplyHolder::ptr()
{
    return reply_ptr;
}

} /* namespace redis */
