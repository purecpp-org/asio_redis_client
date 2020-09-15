//
// Created by qicosmos on 2020/9/9.
//

#ifndef ASIO_REDIS_CLIENT_ASIO_REDIS_CLIENT_H
#define ASIO_REDIS_CLIENT_ASIO_REDIS_CLIENT_H
#include <memory>
#include <deque>
#include <mutex>
#include <boost/asio.hpp>

namespace purecpp {
static const char* CRCF = "\r\n";

class asio_redis_client : public std::enable_shared_from_this<asio_redis_client> {
public:
  asio_redis_client(boost::asio::io_service &ios)
      : ios_(ios), resolver_(ios), socket_(ios) {}

  ~asio_redis_client() { close(); }

  bool connect_with_trytimes(const std::string &host, int port,
                             size_t try_times) {
    for (size_t i = 0; i < try_times+1; ++i) {
      if (connect(host, port)) {
        return true;
      }
    }

    return false;
  }

  bool connect(const std::string &host, int port, size_t timeout_seconds = 3) {
    auto promise = std::make_shared<std::promise<bool>>();
    std::weak_ptr<std::promise<bool>> weak(promise);

    async_connect(host, port, weak);

    auto future = promise->get_future();
    auto status = future.wait_for(std::chrono::seconds(timeout_seconds));
    if (status == std::future_status::timeout) {
      promise = nullptr;
      close();
      return false;
    }

    bool r = future.get();
    promise = nullptr;
    return r;
  }

  void command(const std::string& cmd){
    std::unique_lock<std::mutex> lock(write_mtx_);
    outbox_.emplace_back(cmd);
    if (outbox_.size() > 1) {
      return;
    }

    write();
  }

  template<typename T, typename= typename std::enable_if<std::is_arithmetic<T>::value>::type>
  void set(const std::string& key, const T& value){
    std::vector<std::string> v{"SET", key, std::to_string(value)};
    std::string cmd = make_command(v);
    command(cmd);
  }

  void set(const std::string& key, const std::string& value){
    std::vector<std::string> v{"SET", key, value};
    std::string cmd = make_command(v);
    command(cmd);
  }

  void get(std::string key){
    std::vector<std::string> v{"GET", key};
    std::string cmd = make_command(v);
    command(cmd);
  }

  std::string make_command(const std::vector<std::string> &items)
  {
    std::string result;
    result.append("*").append(std::to_string(items.size())).append(CRCF);

    for(const auto &item: items)
    {
      result.append("$").append(std::to_string(item.size())).append(CRCF);
      result.append(item).append(CRCF);
    }

    return result;
  }

private:
  void async_connect(const std::string &host, int port,
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
                  has_connected_ = true;
                  do_read();
                } else {
                  close();
                }

                auto sp = weak.lock();
                if (sp)
                  sp->set_value(has_connected_);
              });
        });
  }

  void do_read() {
    auto self = shared_from_this();
    async_read_until(CRCF, [this, self](boost::system::error_code ec, size_t size) {
      if (ec) {
        close();
        return;
      }

      const char* data_ptr = boost::asio::buffer_cast<const char*>(read_buf_.data());
      size_t buf_size = read_buf_.size();

      //int ret = parser_.parse_response(data_ptr, size, 0);
      read_buf_.consume(size);

      //do_read_body(parser_.keep_alive(), parser_.status(), size_to_read);
    });
  }

  void write() {
    auto& msg = outbox_[0];
    auto self = shared_from_this();
    async_write(msg, [this, self](const boost::system::error_code& ec, const size_t length) {
      if (ec) {
        //print(ec);
        close();
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

  void close() {
    if (!has_connected_)
      return;

    boost::system::error_code ec;
    has_connected_ = false;
    // timer_.cancel(ec);
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
  }

  template<typename Handler>
  void async_read(size_t size_to_read, Handler handler) {
    boost::asio::async_read(socket_, read_buf_, boost::asio::transfer_exactly(size_to_read), std::move(handler));
  }

  template<typename Handler>
  void async_read_until(const std::string& delim, Handler handler) {
    boost::asio::async_read_until(socket_, read_buf_, delim, std::move(handler));
  }

  template<typename Handler>
  void async_write(const std::string& msg, Handler handler) {
    boost::asio::async_write(socket_, boost::asio::buffer(msg), std::move(handler));
  }

  boost::asio::io_service &ios_;
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::streambuf read_buf_;
  std::atomic_bool has_connected_ = {false};

  std::deque<std::string> outbox_;
  std::mutex write_mtx_;
};
}
#endif // ASIO_REDIS_CLIENT_ASIO_REDIS_CLIENT_H
