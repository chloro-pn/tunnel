#ifndef TUNNEL_SWITCHER_CLIENT_H
#define TUNNEL_SWITCHER_CLIENT_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include "asio.hpp"
#include "async_simple/coro/Lazy.h"
#include "awaiter/asio/socket.h"
#include "switcher/switcher.h"
#include "tunnel/sedeserialize.h"

namespace tunnel {

namespace async_simple_package {

template <typename T>
async_simple::coro::Lazy<T> read_package(asio::ip::tcp::socket& s) {
  using namespace ip::tcp;
  std::array<char, sizeof(uint32_t)> buf;
  co_await SocketReadAwaiter(s, asio::buffer(buf));
  uint32_t length = Deserialize<uint32_t>(std::string_view(&buf[0], buf.size()));
  std::string buffer(length, '/0');
  co_await SocketReadAwaiter(s, asio::buffer(buffer));
  co_return Deserialize<T>(buffer);
}

template <typename T>
async_simple::coro::Lazy<T> read_package(asio::ip::tcp::socket& s, bool& eof) {
  using namespace ip::tcp;
  std::array<char, sizeof(uint32_t)> buf;
  size_t n = co_await SocketReadAwaiter(s, asio::buffer(buf), false);
  if (n != sizeof(uint32_t)) {
    eof = true;
    co_return T{};
  }
  uint32_t length = Deserialize<uint32_t>(std::string_view(&buf[0], buf.size()));
  std::string buffer(length, '/0');
  co_await SocketReadAwaiter(s, asio::buffer(buffer));
  co_return Deserialize<T>(buffer);
}

template <typename T>
async_simple::coro::Lazy<void> write_package(asio::ip::tcp::socket& s, const T& pkg) {
  using namespace ip::tcp;
  std::string buf;
  Serialize(pkg, buf);
  assert(buf.size() <= std::numeric_limits<uint32_t>::max());
  std::string length_buf;
  Serialize(static_cast<uint32_t>(buf.size()), length_buf);
  co_await SocketWriteAwaiter(s, asio::buffer(length_buf));
  co_await SocketWriteAwaiter(s, asio::buffer(buf));
  co_return;
}

}  // namespace async_simple_package

class SwitcherClient {
  using socket = asio::ip::tcp::socket;

 public:
  SwitcherClient(asio::io_context& ctx, const std::string& ip, uint16_t port)
      : ctx_(ctx), ip_(ip), port_(port), connected_(false), socket_(ctx_) {}

  async_simple::coro::Lazy<void> Connect() {
    using namespace ip::tcp;
    using namespace asio::ip;

    if (connected_ == false) {
      bool succ = co_await SocketConnectAwaiter(socket_, tcp::endpoint{address::from_string(ip_), port_});
      if (succ == false) {
        throw std::runtime_error("connect to switcher failed");
      }
      connected_ = true;
    }
    co_return;
  }

  template <typename T>
  async_simple::coro::Lazy<void> Push(const std::string& topic, std::optional<T>&& value) {
    request_package rp;
    rp.topic = topic;
    rp.type = request_type_push;
    co_await async_simple_package::write_package<request_package>(socket_, rp);
    data_package dp;
    if (value.has_value()) {
      Serialize(value.value(), dp.data);
      dp.eof = false;
    } else {
      dp.eof = true;
    }
    co_await async_simple_package::write_package<data_package>(socket_, dp);
    response_package resp = co_await async_simple_package::read_package<response_package>(socket_);
    co_return;
  }

  template <typename T>
  async_simple::coro::Lazy<std::optional<T>> Pop(const std::string& topic) {
    request_package rp;
    rp.topic = topic;
    rp.type = request_type_pop;
    co_await async_simple_package::write_package<request_package>(socket_, rp);
    data_package dp = co_await async_simple_package::read_package<data_package>(socket_);
    response_package resp = co_await async_simple_package::read_package<response_package>(socket_);
    if (dp.eof == true) {
      co_return std::optional<T>{};
    }
    co_return Deserialize<T>(dp.data);
  }

  async_simple::coro::Lazy<int> CloseTopic(const std::string& topic) {
    request_package rp;
    rp.topic = topic;
    rp.type = request_type_close_topic;
    co_await async_simple_package::write_package<request_package>(socket_, rp);
    response_package dp = co_await async_simple_package::read_package<response_package>(socket_);
    co_return dp.response_code;
  }

  async_simple::coro::Lazy<bool> CheckTopicExist(const std::string& topic) {
    request_package rp;
    rp.topic = topic;
    rp.type = request_type_check_topic_exist;
    co_await async_simple_package::write_package<request_package>(socket_, rp);
    response_package dp = co_await async_simple_package::read_package<response_package>(socket_);
    co_return dp.response_code == response_ok;
  }

  ~SwitcherClient() {
    if (connected_ == true) {
      socket_.shutdown(socket::shutdown_send);
      socket_.close();
      connected_ = false;
    }
  }

 private:
  asio::io_context& ctx_;
  std::string ip_;
  uint16_t port_;

  bool connected_;
  socket socket_;
};

}  // namespace tunnel

#endif
