//
// Created by qicosmos on 2020/9/9.
//

#ifndef ASIO_REDIS_CLIENT_ASIO_REDIS_CLIENT_H
#define ASIO_REDIS_CLIENT_ASIO_REDIS_CLIENT_H
#include <memory>
#include <deque>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/utility/string_view.hpp>
#include "parser/redisparser.h"
#include "error_code.h"

#ifdef USE_FUTURE
#include <future/future.h>
#endif

namespace purecpp {
constexpr const char *CRCF = "\r\n";
constexpr const size_t CRCF_SIZE = 2;
using RedisCallback = std::function<void(RedisValue)>;

template<typename... Args>
inline void print(Args&&... args) {
#ifdef DEBUG_INFO
  (void)std::initializer_list<int>{
      (std::cout << std::forward<Args>(args) << ' ', 0)...};
  std::cout << "\n";
#endif
  (void)sizeof...(args);
}

template<typename Container>
inline std::string make_command(const Container &c) {
  std::string result;
  result.append("*").append(std::to_string(c.size())).append(CRCF);

  for (const auto &item : c) {
    result.append("$").append(std::to_string(item.size())).append(CRCF);
    result.append(item).append(CRCF);
  }

  return result;
}

class asio_redis_client
    : public std::enable_shared_from_this<asio_redis_client> {
public:
  asio_redis_client(boost::asio::io_service &ios)
      : ios_(ios), resolver_(ios), socket_(ios) {}

  ~asio_redis_client() { close(); }

  size_t connect_with_trytimes(const std::string &host, unsigned short port,
                             size_t try_times) {
    bool auto_reconnect = enbale_auto_reconnect_;
    enbale_auto_reconnect_ = false;
    size_t has_try = 0;
    for (; has_try < try_times; has_try++) {
      if (connect(host, port)) {
        enbale_auto_reconnect_ = auto_reconnect;
        return has_try;
      }

      reset_socket();

      print("retry times: ", has_try);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    enbale_auto_reconnect_ = auto_reconnect;
    return has_try;
  }

  bool connect(const std::string &host, unsigned short port,
               size_t timeout_seconds = 3) {
    host_ = host;
    port_ = port;
    auto promise = std::make_shared<std::promise<bool>>();
    std::weak_ptr<std::promise<bool>> weak(promise);

    async_connect(host, port, weak);

    auto future = promise->get_future();
    auto status = future.wait_for(std::chrono::seconds(timeout_seconds));
    if (status == std::future_status::timeout) {
      print("connect timeout");
      promise = nullptr;
      close_inner();
      return false;
    }

    bool r = future.get();
    promise = nullptr;
    return r;
  }

#ifdef USE_FUTURE
  Future<RedisValue> auth(const std::string &password) {
    password_ = password;
    std::vector<std::string> v{"AUTH", password};
    return command(make_command(v));
  }

  Future<RedisValue> get(const std::string &key) {
    std::vector<std::string> v{"GET", key};
    return command(make_command(v));
  }

  Future<RedisValue> set(const std::string &key, const std::string &value) {
    std::vector<std::string> v{"SET", key, value};
    return command(make_command(v));
  }

  Future<RedisValue> del(const std::string &key) {
    std::vector<std::string> v{"DEL", key};
    return command(make_command(v));
  }

  Future<RedisValue> ping() {
    std::vector<std::string> v{"PING"};
    return command(make_command(v));
  }

  Future<RedisValue> command(const std::string &cmd, std::deque<std::string> args) {
    args.push_front(cmd);
    return command(make_command(args));
  }

  Future<RedisValue> unsubscribe(const std::string &key) {
    std::vector<std::string> v{"UNSUBSCRIBE", key};
    return command(make_command(v));
  }

  Future<RedisValue> punsubscribe(const std::string &key) {
    std::vector<std::string> v{"PUNSUBSCRIBE", key};
    return command(make_command(v));
  }
#endif

  void command(const std::string &cmd, std::deque<std::string> args, RedisCallback callback){
    args.push_front(cmd);
    return command(make_command(args), std::move(callback));
  }

  template <typename T, typename = typename std::enable_if<
                            std::is_arithmetic<T>::value>::type>
  void set(const std::string &key, const T &value, RedisCallback callback) {
    std::vector<std::string> v{"SET", key, std::to_string(value)};
    command(make_command(v), std::move(callback));
  }

  void set(const std::string &key, const std::string &value,
           RedisCallback callback) {
    std::vector<std::string> v{"SET", key, value};
    command(make_command(v), std::move(callback));
  }

  void del(const std::string &key, RedisCallback callback) {
    std::vector<std::string> v{"DEL", key};
    command(make_command(v), std::move(callback));
  }

  void ping(RedisCallback callback) {
    std::vector<std::string> v{"PING"};
    command(make_command(v), std::move(callback));
  }

  void auth(const std::string &password, RedisCallback callback) {
    password_ = password;
    auth_callback_ = callback;
    std::vector<std::string> v{"AUTH", password};
    command(make_command(v), std::move(callback));
  }

  void get(const std::string &key, RedisCallback callback) {
    std::vector<std::string> v{"GET", key};
    command(make_command(v), std::move(callback));
  }

  void publish(const std::string &channel, const std::string &msg,
               RedisCallback callback) {
    std::vector<std::string> v{"PUBLISH", channel, msg};
    command(make_command(v), std::move(callback));
  }

  void subscribe(const std::string &key, RedisCallback callback) {
    std::vector<std::string> v{"SUBSCRIBE", key};
    command(make_command(v), std::move(callback), key);
  }

  void psubscribe(const std::string &key, RedisCallback callback) {
    std::vector<std::string> v{"PSUBSCRIBE", key};
    command(make_command(v), std::move(callback), key);
  }

  void unsubscribe(const std::string &key, RedisCallback callback) {
    std::vector<std::string> v{"UNSUBSCRIBE", key};
    command(make_command(v), std::move(callback));
  }

  void punsubscribe(const std::string &key, RedisCallback callback) {
    std::vector<std::string> v{"PUNSUBSCRIBE", key};
    command(make_command(v), std::move(callback));
  }

  void enable_auto_reconnect(bool enable) { enbale_auto_reconnect_ = enable; }

  void close() {
    enbale_auto_reconnect_ = false;
    close_inner();
  }

  bool has_connected() const {
    return has_connected_;
  }

private:
  void async_connect(const std::string &host, unsigned short port,
                     std::weak_ptr<std::promise<bool>> weak) {
    boost::asio::ip::tcp::resolver::query query(host, std::to_string(port));
    auto self = this->shared_from_this();
    resolver_.async_resolve(
        query,
        [this, self, weak](boost::system::error_code ec,
                           const boost::asio::ip::tcp::resolver::iterator &it) {
          if (ec) {
            auto sp = weak.lock();
            if (sp) {
              sp->set_value(false);
            }

            return;
          }

          auto self = shared_from_this();
          boost::asio::async_connect(
              socket_, it,
              [this, self,
               weak](boost::system::error_code ec,
                     const boost::asio::ip::tcp::resolver::iterator &) {
                if (!ec) {
                  if (has_connected_) {
                    return;
                  }

                  has_connected_ = true;
                  print("connect ok");
                  resubscribe();
                  do_read();
                } else {
                  print(ec.message());
                  close_inner();
                  if (enbale_auto_reconnect_) {
                    print("auto connect");
                    async_reconnect();
                  }
                }

                auto sp = weak.lock();
                if (sp)
                  sp->set_value(has_connected_);
              });
        });
  }

  void async_reconnect() {
    reset_socket();
    async_connect(host_, port_, {});
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  void resubscribe() {
    if (!password_.empty()) {
      auto self = shared_from_this();
      assert(auth_callback_);
      auth(password_,std::move(auth_callback_));
    }

    if (sub_handlers_.empty()) {
      return;
    }

    for (auto &pair : sub_handlers_) {
      if (pair.first.find("*") != std::string::npos) { // improve later
        psubscribe(pair.first, pair.second);
      } else {
        subscribe(pair.first, pair.second);
      }
    }
  }

  void do_read() {
    auto self = shared_from_this();
    async_read_some([this, self](boost::system::error_code ec, size_t size) {
      if (ec) {
        handle_message(RedisValue(ErrorCode::io_error, ec.message()));
        close_inner();
        if (enbale_auto_reconnect_) {
          async_reconnect();
        }
        return;
      }

      for (size_t pos = 0; pos < size;) {
        std::pair<size_t, RedisParser::ParseResult> result =
            parser_.parse(read_buf_.data() + pos, size - pos);

        if (result.second == RedisParser::Completed) {
          handle_message(parser_.result());
        } else if (result.second == RedisParser::Incompleted) {
          do_read();
          return;
        } else {
          handle_message(RedisValue(ErrorCode::redis_parse_error, "redis parse error"));
          return;
        }

        pos += result.first;
      }

      do_read();
    });
  }

  void close_inner() {
    if (!has_connected_)
      return;

    has_connected_ = false;

    clear_outbox_and_handlers();

    boost::system::error_code ec;
    // timer_.cancel(ec);
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
  }

  void reset_socket() {
    socket_ = decltype(socket_)(ios_);
    if (!socket_.is_open()) {
      socket_.open(boost::asio::ip::tcp::v4());
    }
  }

  bool is_subscribe(const std::string &cmd) {
    if (cmd == "subscribe" || cmd == "psubscribe" || cmd == "message" ||
        cmd == "pmessage") {
      return true;
    }

    return false;
  }

  void handle_array_msg(RedisValue v) {
    std::vector<RedisValue> array = v.toArray();
    auto &value = array[0];
    std::string cmd = value.toString();
    if (is_subscribe(cmd)) {
      if (array.size() < 3) {
        // error, not redis protocol
        return;
      }

      handle_subscribe_msg(std::move(cmd), std::move(array));
    } else {
      handle_non_subscribe_msg(std::move(v));
    }
  }

  void handle_subscribe_msg(std::string cmd, std::vector<RedisValue> array) {
    std::string subscribe_key = array[1].toString();
    RedisValue value;
    if (cmd == "subscribe" || cmd == "psubscribe") {
      // reply subscribe
//      std::cout << cmd << " ok\n";
      print(cmd);
      return;
    } else if (cmd == "message") {
      value = std::move(array[2]);
    } else { // pmessage
      value = std::move(array[3]);
    }

    std::function<void(RedisValue)> *callback = nullptr;
    {
      assert(!sub_handlers_.empty());
      auto it = sub_handlers_.find(subscribe_key);
      if (it != sub_handlers_.end()) {
        callback = &it->second;
      }else{
        print("subscibe key: ", subscribe_key, " not found");
      }
    }

    if (callback) {
      try {
        auto &cb = *callback;
        if (cb) {
          cb(std::move(value));
        }
      } catch (std::exception &e) {
        print(e.what());
      } catch (...) {
        print("unknown exception");
      }
    }
  }

  void handle_non_subscribe_msg(RedisValue value) {
    std::function<void(RedisValue)> front = nullptr;
    {
      std::unique_lock<std::mutex> lock(write_mtx_);
      if (handlers_.empty()) {
        print("warning! no handler deal with this value : ", value.inspect());
        return;
      }

      front = std::move(handlers_.front());
      handlers_.pop_front();
    }

    try {
      if (front) {
        front(std::move(value));
      }
    } catch (std::exception &e) {
      print(e.what());
    } catch (...) {
      print("unknown exception");
    }
  }

  void handle_message(RedisValue v) {
    if (v.isArray()) {
      handle_array_msg(std::move(v));
    } else {
      handle_non_subscribe_msg(std::move(v));
    }
  }

  void write() {
    auto &msg = outbox_[0];
    auto self = shared_from_this();
    async_write(msg, [this, self](const boost::system::error_code &ec, size_t) {
      if (ec) {
        print(ec.message());
        close_inner();
        return;
      }

      std::unique_lock<std::mutex> lock(write_mtx_);
      if (outbox_.empty()) {
        return;
      }

      outbox_.pop_front();

      if (!outbox_.empty()) {
        // more messages to send
        write();
      }
    });
  }

  void clear_outbox_and_handlers() {
    std::unique_lock<std::mutex> lock(write_mtx_);
    if (!handlers_.empty()) {
      handlers_.clear();
    }
    if(!outbox_.empty()){
      outbox_.clear();
    }
  }

  template <typename Handler> void async_read_some(Handler handler) {
    socket_.async_read_some(boost::asio::buffer(read_buf_), std::move(handler));
  }

  template <typename Handler>
  void async_write(const std::string &msg, Handler handler) {
    boost::asio::async_write(socket_, boost::asio::buffer(msg),
                             std::move(handler));
  }

  void command(const std::string &cmd, RedisCallback callback,
                     std::string sub_key = "") {
    std::unique_lock<std::mutex> lock(write_mtx_);
    outbox_.emplace_back(cmd);
    if (sub_key.empty()) {
      handlers_.emplace_back(std::move(callback));
    } else {
      sub_handlers_.emplace(std::move(sub_key), std::move(callback));
    }
    print("add handler");//for debug
    if (outbox_.size() > 1) {
      return;
    }

    write();
  }

#ifdef USE_FUTURE
  Future<RedisValue> command(const std::string &cmd) {
    if (!has_connected_) {
      return {};
    }

    std::shared_ptr<purecpp::Promise<RedisValue>> promise =
            std::make_shared<purecpp::Promise<RedisValue>>();
    auto callback = [promise](RedisValue value) {
        promise->SetValue(std::move(value));
    };

    {
      std::unique_lock<std::mutex> lock(write_mtx_);
      outbox_.emplace_back(cmd);
      handlers_.emplace_back(std::move(callback));

      if (outbox_.size() <= 1) {
        write();
      }
    }

    return promise->GetFuture();
  }
#endif

  boost::asio::io_service &ios_;
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::ip::tcp::socket socket_;
  std::atomic_bool has_connected_ = {false};

  std::deque<std::string> outbox_;
  std::mutex write_mtx_;
  std::array<char, 4096> read_buf_;
  RedisParser parser_;
  std::deque<std::function<void(RedisValue)>> handlers_;
  std::map<std::string, std::function<void(RedisValue)>> sub_handlers_;
  bool enbale_auto_reconnect_ = false;
  std::string host_;
  unsigned short port_;
  std::string password_;
  RedisCallback auth_callback_ = nullptr;
};
}
#endif // ASIO_REDIS_CLIENT_ASIO_REDIS_CLIENT_H
