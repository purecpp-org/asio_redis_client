/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISVALUE_CPP
#define REDISCLIENT_REDISVALUE_CPP

#include <string.h>

#include "redisvalue.h"

namespace purecpp {

RedisValue::RedisValue()
    : value_(NullTag()), error_(false)
{
}

RedisValue::RedisValue(RedisValue &&other)
    : value_(std::move(other.value_)), error_(other.error_), error_code_(other.error_code_)
{
}

RedisValue::RedisValue(int64_t i)
    : value_(i), error_(false)
{
}

RedisValue::RedisValue(const char *s)
    : value_(std::vector<char>(s, s + strlen(s)) ), error_(false)
{
}

RedisValue::RedisValue(const std::string &s)
    : value_(std::vector<char>(s.begin(), s.end()) ), error_(false)
{
}

RedisValue::RedisValue(std::vector<char> buf)
    : value_(std::move(buf)), error_(false)
{
}

RedisValue::RedisValue(std::vector<char> buf, struct ErrorTag)
    : value_(std::move(buf)), error_(true)
{
}

RedisValue::RedisValue(std::vector<RedisValue> array)
    : value_(std::move(array)), error_(false)
{
}

RedisValue::RedisValue(int error_code, const std::string& error_msg)
    : value_(std::vector<char>(error_msg.begin(), error_msg.end())), error_(true), error_code_(error_code)
{

}

std::vector<RedisValue> RedisValue::toArray() const
{
    return castTo< std::vector<RedisValue> >();
}

std::string RedisValue::toString() const
{
    const std::vector<char> &buf = toByteArray();
    return std::string(buf.begin(), buf.end());
}

std::vector<char> RedisValue::toByteArray() const
{
    return castTo<std::vector<char> >();
}

int64_t RedisValue::toInt() const
{
    return castTo<int64_t>();
}

std::string RedisValue::inspect() const
{
    if( isError() )
    {
        static std::string err = "error: ";
        std::string result;

        result = err;
        result += toString();

        return result;
    }
    else if( isNull() )
    {
        static std::string null = "(null)";
        return null;
    }
    else if( isInt() )
    {
        return std::to_string(toInt());
    }
    else if( isString() )
    {
        return toString();
    }
    else
    {
        std::vector<RedisValue> values = toArray();
        std::string result = "[";

        if( values.empty() == false )
        {
            for(size_t i = 0; i < values.size(); ++i)
            {
                result += values[i].inspect();
                result += ", ";
            }

            result.resize(result.size() - 1);
            result[result.size() - 1] = ']';
        }
        else
        {
            result += ']';
        }

        return result;
    }
}

bool RedisValue::isOk() const
{
    return !isError();
}

bool RedisValue::isError() const
{
    return error_;
}

bool RedisValue::isNull() const
{
    return typeEq<NullTag>();
}

bool RedisValue::isInt() const
{
    return typeEq<int64_t>();
}

bool RedisValue::isString() const
{
    return typeEq<std::vector<char> >();
}

bool RedisValue::isByteArray() const
{
    return typeEq<std::vector<char> >();
}

bool RedisValue::isArray() const
{
    return typeEq< std::vector<RedisValue> >();
}

std::vector<char> &RedisValue::getByteArray()
{
    assert(isByteArray());
    return boost::get<std::vector<char>>(value_);
}

const std::vector<char> &RedisValue::getByteArray() const
{
    assert(isByteArray());
    return boost::get<std::vector<char>>(value_);
}

std::vector<RedisValue> &RedisValue::getArray()
{
    assert(isArray());
    return boost::get<std::vector<RedisValue>>(value_);
}

const std::vector<RedisValue> &RedisValue::getArray() const
{
    assert(isArray());
    return boost::get<std::vector<RedisValue>>(value_);
}

bool RedisValue::operator == (const RedisValue &rhs) const
{
    return value_ == rhs.value_;
}

bool RedisValue::operator != (const RedisValue &rhs) const
{
    return !(value_ == rhs.value_);
}

}

#endif // REDISCLIENT_REDISVALUE_CPP

