#ifndef TUNNEL_SWITCHER_H
#define TUNNEL_SWITCHER_H

#include <array>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "asio.hpp"
#include "tunnel/sedeserialize.h"

namespace tunnel {

constexpr int request_type_push = 0x00;
constexpr int request_type_pop = 0x01;
constexpr int request_type_check_topic_exist = 0x02;

struct request_package {
  std::string topic;
  int type;  // push, pop, check, query, ...
  // will add more information, like push/pop_ip:port、push/pop pipeline name etc for debug
};

template <>
inline void Serialize(const request_package& v, std::string& appender) {
  Serialize(v.topic, appender);
  Serialize(v.type, appender);
}

template <>
inline request_package Deserialize(std::string_view view, size_t& offset) {
  request_package rp;
  rp.topic = Deserialize<std::string>(view, offset);
  rp.type = Deserialize<int>(view, offset);
  return rp;
}

struct response_package {
  std::string topic;
  // only for push and pop request
  int wait_ms;
  // only for push and pop request
  std::string peer_addr;
};

template <>
inline void Serialize(const response_package& v, std::string& appender) {
  Serialize(v.topic, appender);
  Serialize(v.wait_ms, appender);
  Serialize(v.peer_addr, appender);
}

template <>
inline response_package Deserialize(std::string_view view, size_t& offset) {
  response_package rp;
  rp.topic = Deserialize<std::string>(view, offset);
  rp.wait_ms = Deserialize<int>(view, offset);
  rp.peer_addr = Deserialize<std::string>(view, offset);
  return rp;
}

struct data_package {
  std::string data;
  std::string meta_info;
};

template <>
inline void Serialize(const data_package& v, std::string& appender) {
  Serialize(v.data, appender);
  Serialize(v.meta_info, appender);
}

template <>
inline data_package Deserialize(std::string_view view, size_t& offset) {
  data_package dp;
  dp.data = Deserialize<std::string>(view, offset);
  dp.meta_info = Deserialize<std::string>(view, offset);
  return dp;
}

template <typename T>
asio::awaitable<T> read_package(asio::ip::tcp::socket& s) {
  std::array<char, sizeof(uint32_t)> buf;
  co_await asio::async_read(s, asio::buffer(buf), asio::use_awaitable);
  uint32_t length = Deserialize<uint32_t>(std::string_view(&buf[0], buf.size()));
  std::string buffer(length, '/0');
  co_await asio::async_read(s, asio::buffer(buffer), asio::use_awaitable);
  co_return Deserialize<T>(buffer);
}

template <typename T>
asio::awaitable<T> read_package(asio::ip::tcp::socket& s, bool& eof) {
  asio::error_code ec;
  std::array<char, sizeof(uint32_t)> buf;
  co_await asio::async_read(s, asio::buffer(buf), asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    if (ec == asio::error::eof) {
      eof = true;
      co_return T{};
    } else {
      throw std::runtime_error(ec.message());
    }
  }
  uint32_t length = Deserialize<uint32_t>(std::string_view(&buf[0], buf.size()));
  std::string buffer(length, '/0');
  co_await asio::async_read(s, asio::buffer(buffer), asio::use_awaitable);
  auto v = Deserialize<T>(buffer);
  co_return v;
}

template <typename T>
asio::awaitable<void> write_package(asio::ip::tcp::socket& s, const T& pkg) {
  std::string buf;
  Serialize(pkg, buf);
  assert(buf.size() <= std::numeric_limits<uint32_t>::max());
  std::string length_buf;
  Serialize(static_cast<uint32_t>(buf.size()), length_buf);
  co_await asio::async_write(s, asio::buffer(length_buf), asio::use_awaitable);
  co_await asio::async_write(s, asio::buffer(buf), asio::use_awaitable);
  co_return;
}

class Switcher {
  using socket = asio::ip::tcp::socket;
  using acceptor = asio::ip::tcp::acceptor;
  using endpoint = asio::ip::tcp::endpoint;

 public:
  explicit Switcher(endpoint ep) : ctx_(), guard_(asio::make_work_guard(ctx_)), ep_(ep) {
    worker_ = std::thread([&] {
      try {
        this->ctx_.run();
      } catch (const std::exception& e) {
        std::printf("io_context exception : %s\n", e.what());
        std::exit(-1);
      }
    });
  }

  void Start() { asio::co_spawn(ctx_, start_accept(), asio::detached); }

  void Stop() {
    guard_.reset();
    ctx_.stop();
    worker_.join();
  }

 private:
  asio::awaitable<void> start_accept() {
    acceptor accept(ctx_, ep_);
    while (true) {
      socket s = co_await accept.async_accept(asio::use_awaitable);
      asio::co_spawn(ctx_, handle(std::move(s)), asio::detached);
    }
    co_return;
  }

  asio::awaitable<void> handle(socket s) {
    bool eof = false;
    request_package pkg = co_await read_package<request_package>(s, eof);
    if (eof == true) {
      // closed by client
      co_return;
    }
    if (pkg.type == request_type_push) {
      handle_push_request(std::move(s), pkg);
    } else if (pkg.type == request_type_pop) {
      handle_pop_request(std::move(s), pkg);
    } else {
      throw std::runtime_error("invalid request type for switcher now.");
    }
    co_return;
  }

  void handle_push_request(socket s, const request_package& pkg) {
    auto it = datas_.find(pkg.topic);
    if (it == datas_.end() || it->second.poping_nodes.empty()) {
      datas_[pkg.topic].pushing_nodes.emplace_back(std::make_unique<socket>(std::move(s)));
    } else {
      auto socket_ptr = std::move(it->second.poping_nodes.back());
      it->second.poping_nodes.pop_back();
      socket& pop_socket = *socket_ptr;
      asio::co_spawn(ctx_, transfer_data(std::move(s), std::move(pop_socket), pkg.topic), asio::detached);
    }
  }

  void handle_pop_request(socket s, request_package pkg) {
    auto it = datas_.find(pkg.topic);
    if (it == datas_.end() || it->second.pushing_nodes.empty()) {
      datas_[pkg.topic].poping_nodes.emplace_back(std::make_unique<socket>(std::move(s)));
    } else {
      auto socket_ptr = std::move(it->second.pushing_nodes.back());
      it->second.pushing_nodes.pop_back();
      socket& push_socket = *socket_ptr;
      asio::co_spawn(ctx_, transfer_data(std::move(push_socket), std::move(s), pkg.topic), asio::detached);
    }
  }

  // read data from push_socket and write to pop_socket
  asio::awaitable<void> transfer_data(socket push_socket, socket pop_socket, std::string topic) {
    data_package dpkg = co_await read_package<data_package>(push_socket);
    co_await write_package<data_package>(pop_socket, dpkg);
    response_package rpkg;
    rpkg.peer_addr = get_remote_ipport(pop_socket);
    rpkg.topic = topic;
    rpkg.wait_ms = 0;  // todo
    co_await write_package<response_package>(push_socket, rpkg);
    rpkg.peer_addr = get_remote_ipport(push_socket);
    co_await write_package<response_package>(pop_socket, rpkg);
    // try to handle next request
    asio::co_spawn(ctx_, handle(std::move(push_socket)), asio::detached);
    asio::co_spawn(ctx_, handle(std::move(pop_socket)), asio::detached);
    co_return;
  }

  std::string get_local_ipport(socket& s) {
    return s.local_endpoint().address().to_string() + ":" + std::to_string(s.local_endpoint().port());
  }

  std::string get_remote_ipport(socket& s) {
    return s.remote_endpoint().address().to_string() + ":" + std::to_string(s.remote_endpoint().port());
  }

 private:
  struct pending_nodes {
    std::vector<std::unique_ptr<socket>> pushing_nodes;
    std::vector<std::unique_ptr<socket>> poping_nodes;
  };

  asio::io_context ctx_;
  asio::executor_work_guard<asio::io_context::executor_type> guard_;
  std::thread worker_;
  endpoint ep_;
  std::unordered_map<std::string, pending_nodes> datas_;
};

}  // namespace tunnel

#endif
