/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISVALUE_H
#define REDISCLIENT_REDISVALUE_H

#include <boost/variant.hpp>
#include <string>
#include <vector>

#include "config.h"

namespace purecpp {

  class RedisValue {
  public:
    struct ErrorTag {
    };

    REDIS_CLIENT_DECL RedisValue()
            : value_(NullTag()), error_(false) {
    }

    REDIS_CLIENT_DECL RedisValue(RedisValue &&other)
            : value_(std::move(other.value_)), error_(other.error_), error_code_(other.error_code_) {
    }

    REDIS_CLIENT_DECL RedisValue(int64_t i)
            : value_(i), error_(false) {
    }

    REDIS_CLIENT_DECL RedisValue(const char *s)
            : value_(std::vector<char>(s, s + strlen(s))), error_(false) {
    }

    REDIS_CLIENT_DECL RedisValue(const std::string &s)
            : value_(std::vector<char>(s.begin(), s.end())), error_(false) {
    }

    REDIS_CLIENT_DECL RedisValue(std::vector<char> buf)
            : value_(std::move(buf)), error_(false) {
    }

    REDIS_CLIENT_DECL RedisValue(std::vector<char> buf, struct ErrorTag)
            : value_(std::move(buf)), error_(true) {
    }

    REDIS_CLIENT_DECL RedisValue(std::vector<RedisValue> array)
            : value_(std::move(array)), error_(false) {
    }

    RedisValue(int error_code, const std::string &error_msg)
            : value_(std::vector<char>(error_msg.begin(), error_msg.end())), error_(true), error_code_(error_code) {

    }

    RedisValue(const RedisValue &) = default;

    RedisValue &operator=(const RedisValue &) = default;

    RedisValue &operator=(RedisValue &&) = default;

    // Return the value as a std::string if
    // type is a byte string; otherwise returns an empty std::string.
    REDIS_CLIENT_DECL std::string toString() const {
      const std::vector<char> &buf = toByteArray();
      return std::string(buf.begin(), buf.end());
    }

    // Return the value as a std::vector<char> if
    // type is a byte string; otherwise returns an empty std::vector<char>.
    REDIS_CLIENT_DECL std::vector<char> toByteArray() const {
      return castTo<std::vector<char> >();
    }

    // Return the value as a std::vector<RedisValue> if
    // type is an int; otherwise returns 0.
    REDIS_CLIENT_DECL int64_t toInt() const {
      return castTo<int64_t>();
    }

    // Return the value as an array if type is an array;
    // otherwise returns an empty array.
    REDIS_CLIENT_DECL std::vector<RedisValue> toArray() const {
      return castTo<std::vector<RedisValue> >();
    }

    // Return the string representation of the value. Use
    // for dump content of the value.
    REDIS_CLIENT_DECL std::string inspect() const {
      if (isError()) {
        static std::string err = "error: ";
        std::string result;

        result = err;
        result += toString();

        return result;
      } else if (isNull()) {
        static std::string null = "(null)";
        return null;
      } else if (isInt()) {
        return std::to_string(toInt());
      } else if (isString()) {
        return toString();
      } else {
        std::vector<RedisValue> values = toArray();
        std::string result = "[";

        if (values.empty() == false) {
          for (size_t i = 0; i < values.size(); ++i) {
            result += values[i].inspect();
            result += ", ";
          }

          result.resize(result.size() - 1);
          result[result.size() - 1] = ']';
        } else {
          result += ']';
        }

        return result;
      }
    }

    // Return true if value not a error_
    REDIS_CLIENT_DECL bool isOk() const {
      return !isError();
    }
    // Return true if value is a error_
    REDIS_CLIENT_DECL bool isError() const {
      return error_;
    }

    bool IsIOError() const {
      return error_code_ == 1;
    }

    int ErrorCode() {
      return error_code_;
    }

    // Return true if this is a null.
    REDIS_CLIENT_DECL bool isNull() const {
      return typeEq<NullTag>();
    }
    // Return true if type is an int
    REDIS_CLIENT_DECL bool isInt() const {
      return typeEq<int64_t>();
    }
    // Return true if type is an array
    REDIS_CLIENT_DECL bool isArray() const {
      return typeEq<std::vector<RedisValue> >();
    }
    // Return true if type is a string/byte array. Alias for isString();
    REDIS_CLIENT_DECL bool isByteArray() const {
      return typeEq<std::vector<char> >();
    }
    // Return true if type is a string/byte array. Alias for isByteArray().
    REDIS_CLIENT_DECL bool isString() const {
      return typeEq<std::vector<char> >();
    }

    // Methods for increasing perfomance
    // Throws: boost::bad_get if the type does not match
    REDIS_CLIENT_DECL std::vector<char> &getByteArray() {
      assert(isByteArray());
      return boost::get<std::vector<char>>(value_);
    }

    REDIS_CLIENT_DECL const std::vector<char> &getByteArray() const {
      assert(isByteArray());
      return boost::get<std::vector<char>>(value_);
    }

    REDIS_CLIENT_DECL std::vector<RedisValue> &getArray() {
      assert(isArray());
      return boost::get<std::vector<RedisValue>>(value_);
    }

    REDIS_CLIENT_DECL const std::vector<RedisValue> &getArray() const {
      assert(isArray());
      return boost::get<std::vector<RedisValue>>(value_);
    }


    REDIS_CLIENT_DECL bool operator==(const RedisValue &rhs) const {
      return value_ == rhs.value_;
    }

    REDIS_CLIENT_DECL bool operator!=(const RedisValue &rhs) const {
      return !(value_ == rhs.value_);
    }

  protected:
    template<typename T>
    T castTo() const;

    template<typename T>
    bool typeEq() const;

  private:
    struct NullTag {
      inline bool operator==(const NullTag &) const {
        return true;
      }
    };

    boost::variant<NullTag, int64_t, std::vector<char>, std::vector<RedisValue> > value_;
    bool error_;
    int error_code_ = 0;
  };


  template<typename T>
  T RedisValue::castTo() const {
    if (value_.type() == typeid(T))
      return boost::get<T>(value_);
    else
      return T();
  }

  template<typename T>
  bool RedisValue::typeEq() const {
    if (value_.type() == typeid(T))
      return true;
    else
      return false;
  }

}

#endif // REDISCLIENT_REDISVALUE_H
