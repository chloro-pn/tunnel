#ifndef TUNNEL_SOCKET_SOURCE_H
#define TUNNEL_SOCKET_SOURCE_H

#include <cassert>
#include <string>

#include "awaiter/asio/socket.h"
#include "tunnel/package.h"
#include "tunnel/source.h"
#include "tunnel/tunnel_traits.h"

namespace tunnel {

template <typename T>
requires HasTunnelSerializeSpecialization<T> && HasTunnelDeserializeSpecialization<T>
class SocketSource : public Source<T> {
 public:
  SocketSource(asio::io_context& ctx, const std::string& ip, uint16_t port)
      : ctx_(ctx), ip_(ip), port_(port), socket_(ctx_), connected_(false) {}

 protected:
  virtual async_simple::coro::Lazy<std::optional<T>> generate() override {
    if (connected_ == false) {
      asio::ip::tcp::acceptor acc(ctx_, asio::ip::tcp::endpoint(asio::ip::address::from_string(ip_), port_));
      socket_ = co_await ip::tcp::SocketAcceptAwaiter(acc);
      acc.close();
      connected_ = true;
    }
    std::optional<T> value = co_await Read();
    co_return value;
  }

  virtual async_simple::coro::Lazy<void> after_work() override {
    CloseSocketIfNot();
    co_return;
  }

  virtual async_simple::coro::Lazy<void> hosted_mode() override {
    CloseSocketIfNot();
    Channel<T>& output = this->GetOutputPort();
    co_await this->close_output(output);
    co_return;
  }

  async_simple::coro::Lazy<std::optional<T>> Read() {
    std::string buf;
    buf.resize(sizeof(uint32_t), '0');
    size_t n = co_await ip::tcp::SocketReadAwaiter(socket_, asio::buffer(buf));
    if (n != sizeof(uint32_t)) {
      throw std::runtime_error("socket source read package error");
    }
    uint32_t length = Deserialize<uint32_t>(buf);
    std::string databuf(length, '0');
    n = co_await ip::tcp::SocketReadAwaiter(socket_, asio::buffer(databuf));
    if (n != databuf.size()) {
      throw std::runtime_error("socket source read package error");
    }
    package pkg = Deserialize<package>(databuf);
    if (pkg.eof) {
      co_return std::optional<T>{};
    }
    T v = Deserialize<T>(pkg.bin_data);
    co_return std::optional<T>(std::move(v));
  }

 private:
  asio::io_context& ctx_;
  std::string ip_;
  uint16_t port_;
  asio::ip::tcp::socket socket_;
  bool connected_;

  void CloseSocketIfNot() {
    if (connected_ == true) {
      assert(socket_.is_open());
      socket_.close();
      connected_ = false;
    }
  }
};

}  // namespace tunnel

#endif
