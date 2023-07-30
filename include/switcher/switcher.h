#ifndef TUNNEL_SWITCHER_H
#define TUNNEL_SWITCHER_H

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "asio.hpp"
#include "tunnel/sedeserialize.h"

namespace tunnel {

constexpr int request_type_push = 0x00;
constexpr int request_type_pop = 0x01;

struct request_package {
  int type;  // push, pop, check, query, ...
  std::string topic;
  // for push and pop message, meta_info == "sdk_ip:sdk_port"
  std::string meta_info;
};

struct response_package {
  int type;
  std::string topic;
  int role;               // for type == push and pop, role == 0 -> client, role == 1 -> server
  std::string meta_info;  // for type == push and pop, meta_info == server's ip:port.
};

struct pending_process {
  std::unique_ptr<asio::ip::tcp::socket> socket;
  std::string ipport;  // "ip:port"
};

class Switcher {
  using socket = asio::ip::tcp::socket;
  using acceptor = asio::ip::tcp::acceptor;
  using endpoint = asio::ip::tcp::endpoint;

 public:
  Switcher(asio::io_context& ctx) : ctx_(ctx) {}

  void Start() { asio::co_spawn(ctx_, start_accept(), asio::detached); }

  void Stop() {
    // todo
  }

 private:
  asio::awaitable<void> start_accept() {
    acceptor accept(ctx_, ep_);
    while (true) {
      socket s = co_await accept.async_accept(asio::use_awaitable);
      asio::co_spawn(ctx_, handle(std::move(s)), asio::detached);
    }
  }

  asio::awaitable<void> handle(socket&& s) {
    // read_request
    request_package pkg = co_await read_package<request_package>(s);
    if (pkg.type == request_type_push) {
      handle_push_request(std::move(s), pkg);
    } else if (pkg.type == request_type_pop) {
      handle_pop_request(std::move(s), pkg);
    } else {
      throw std::runtime_error("invalid request type for switcher now.");
    }
  }

  void handle_push_request(socket&& s, const request_package& pkg) {
    auto it = pop_requests_.find(pkg.topic);
    if (it == pop_requests_.end() || it->second.empty()) {
      pending_process pp;
      pp.ipport = pkg.meta_info;
      pp.socket = std::make_unique<socket>(std::move(s));
      push_requests_[pkg.topic].emplace_back(std::move(pp));
    } else {
      auto popper = std::move(it->second.back());
      pop_requests_[pkg.topic].pop_back();
      co_spawn(ctx_, create_channel(pkg.topic, pkg.meta_info, popper.ipport, std::move(s), std::move(*popper.socket)),
               asio::detached);
    }
  }

  void handle_pop_request(socket&& s, const request_package& pkg) {
    auto it = push_requests_.find(pkg.topic);
    if (it == push_requests_.end() || it->second.empty()) {
      pending_process pp;
      pp.ipport = pkg.meta_info;
      pp.socket = std::make_unique<socket>(std::move(s));
      pop_requests_[pkg.topic].emplace_back(std::move(pp));
    } else {
      auto pusher = std::move(it->second.back());
      push_requests_[pkg.topic].pop_back();
      co_spawn(ctx_, create_channel(pkg.topic, pusher.ipport, pkg.meta_info, std::move(*pusher.socket), std::move(s)),
               asio::detached);
    }
  }

  template <typename T>
  asio::awaitable<T> read_package(socket& s) {
    std::array<char, sizeof(uint32_t)> buf;
    co_await asio::read(s, asio::buffer(buf), asio::use_awaitable);
    uint32_t length = Deserialize<uint32_t>(std::string_view(&buf[0], buf.size()));
    std::string buffer(length, '/0');
    co_await asio::read(s, asio::buffer(buffer), asio::use_awaitable);
    co_return Deserialize<T>(buffer);
  }

  template <typename T>
  asio::awaitable<void> write_package(socket& s, const T& pkg) {
    auto buf = Serialize<T>(pkg);
    auto length_buf = Serialize<uint32_t>(buf.size());
    co_await asio::write(s, asio::buffer(length_buf), asio::use_awaitable);
    co_await asio::write(s, asio::buffer(buf), asio::use_awaitable);
    co_return;
  }

  asio::awaitable<void> create_channel(const std::string& topic, const std::string& push_ip_port,
                                       const std::string& pop_ip_port, socket&& push_socket, socket&& pop_socket) {
    response_package rpkg;
    rpkg.type = request_type_pop;
    rpkg.role = 1;
    rpkg.topic = topic;
    rpkg.meta_info = pop_ip_port;
    co_await write_package<response_package>(pop_socket, rpkg);
    pop_socket.shutdown(socket::shutdown_send);
    pop_socket.close();
    rpkg.role = 0;
    rpkg.type = request_type_push;
    co_await write_package<response_package>(push_socket, rpkg);
    push_socket.shutdown(socket::shutdown_send);
    push_socket.close();
    co_return;
  }

  std::string get_local_ipport(socket& s) {
    s.local_endpoint().address().to_string() + ":" + std::to_string(s.local_endpoint().port());
  }

  std::string get_remote_ipport(socket& s) {
    s.remote_endpoint().address().to_string() + ":" + std::to_string(s.remote_endpoint().port());
  }

 private:
  asio::io_context& ctx_;
  endpoint ep_;
  std::unordered_map<std::string, std::vector<pending_process>> push_requests_;
  std::unordered_map<std::string, std::vector<pending_process>> pop_requests_;
};

}  // namespace tunnel

#endif
