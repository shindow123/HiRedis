/*
 * CmdBuilder.h
 *
 *  Created on: 2016年6月30日
 *      Author: sangechen
 */

#ifndef SHARE_FW_REDIS_CMDBUILDER_H_
#define SHARE_FW_REDIS_CMDBUILDER_H_

#include <string>
#include <vector>

#include <fmt/format.h>
#include <boost/lexical_cast.hpp>

namespace redis
{

/**
 * 最终目的是要build出 vector<string>
 */
class CmdBuilder
{
protected:
    std::vector<std::string> cmd_args;

public:
    /** 方便将 *this 直接作为std::vector<std::string>类型使用 */
    inline operator const std::vector<std::string>&()
    {
        return cmd_args;
    }

public:
    /**
     * @see: https://en.wikipedia.org/wiki/Variadic_template
     *
     * @see: http://en.cppreference.com/w/cpp/language/direct_initialization
     * 2) during list-initialization sequence,
     *     if no initializer-list constructors are provided
     *     and a matching constructor is accessible,
     *     and all necessary implicit conversions are non-narrowing.
     */
    template <class... Args>
    CmdBuilder(Args&&... args) : cmd_args({boost::lexical_cast<std::string>(std::forward<Args>(args))...})
    {
    }
    virtual ~CmdBuilder();

    /** 可以作为copy-constructor使用 */
    CmdBuilder& append(const std::vector<std::string>& vecArgs)
    {
        cmd_args.insert(cmd_args.end(), vecArgs.begin(), vecArgs.end());
        return *this;
    }

    template <class... Args>
    CmdBuilder& append_args(Args&&... args)
    {
        cmd_args.insert(cmd_args.end(), {boost::lexical_cast<std::string>(std::forward<Args>(args))...});
        return *this;
    }

    /**
     * @see: https://github.com/gabime/spdlog/blob/master/include/spdlog/details/logger_impl.h#L62
     */
    template <class... Args>
    CmdBuilder& append_fmt(Args&&... args)
    {
        cmd_args.push_back(fmt::format(std::forward<Args>(args)...));
        return *this;
    }
};

} /* namespace redis */

#endif /* SHARE_FW_REDIS_CMDBUILDER_H_ */
