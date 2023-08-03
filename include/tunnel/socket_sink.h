#ifndef TUNNEL_SOCKET_SINK_H
#define TUNNEL_SOCKET_SINK_H

#include <cassert>
#include <chrono>
#include <string>

#include "awaiter/asio/socket.h"
#include "tunnel/package.h"
#include "tunnel/sedeserialize.h"
#include "tunnel/sink.h"

namespace tunnel {

template <typename T>
requires HasTunnelSerializeSpecialization<T> && HasTunnelDeserializeSpecialization<T>
class SocketSink : public Sink<T> {
 public:
  SocketSink(asio::io_context& ctx, const std::string& ip, uint16_t port)
      : ctx_(ctx), ip_(ip), port_(port), socket_(ctx_), connected_(false) {}

 protected:
  virtual async_simple::coro::Lazy<void> consume(T&& value) override {
    if (connected_ == false) {
      size_t retry_count = 0;
      bool succ = false;
      using namespace std::chrono_literals;
      std::array<std::chrono::milliseconds, 5> wait_ms{10ms, 200ms, 1000ms, 5000ms, 30000ms};
      while (retry_count < 5) {
        succ = co_await ip::tcp::SocketConnectAwaiter(
            socket_, asio::ip::tcp::endpoint{asio::ip::address::from_string(ip_), port_});
        if (succ == true) {
          break;
        }
        co_await async_simple::coro::sleep(wait_ms[retry_count]);
        socket_ = asio::ip::tcp::socket(socket_.get_executor());
        retry_count += 1;
      }
      if (succ == false) {
        throw std::runtime_error("socket sink connect error");
      }
      connected_ = true;
    }
    co_await Write(std::move(value));
  }

  async_simple::coro::Lazy<void> Write(T&& value) {
    package pkg;
    pkg.eof = false;
    Serialize<T>(value, pkg.bin_data);
    std::string buf;
    Serialize<package>(pkg, buf);
    uint32_t length = buf.size();
    std::string length_buf;
    Serialize<uint32_t>(length, length_buf);
    co_await ip::tcp::SocketWriteAwaiter(socket_, asio::buffer(length_buf));
    co_await ip::tcp::SocketWriteAwaiter(socket_, asio::buffer(buf));
    co_return;
  }

  async_simple::coro::Lazy<void> WriteEof() {
    package pkg;
    pkg.eof = true;
    std::string buf;
    Serialize<package>(pkg, buf);
    uint32_t length = buf.size();
    std::string length_buf;
    Serialize<uint32_t>(length, length_buf);
    co_await ip::tcp::SocketWriteAwaiter(socket_, asio::buffer(length_buf));
    co_await ip::tcp::SocketWriteAwaiter(socket_, asio::buffer(buf));
    co_return;
  }

  virtual async_simple::coro::Lazy<void> after_work() override {
    if (connected_) {
      co_await WriteEof();
      assert(socket_.is_open());
      socket_.close();
      connected_ = false;
    }
    co_return;
  }

  virtual async_simple::coro::Lazy<void> hosted_mode() override {
    if (connected_) {
      assert(socket_.is_open());
      socket_.close();
      connected_ = false;
    }
    Channel<T>& input = this->GetInputPort();
    co_await this->close_input(input, this->input_count_);
    co_return;
  }

 private:
  asio::io_context& ctx_;
  std::string ip_;
  uint16_t port_;
  asio::ip::tcp::socket socket_;
  bool connected_;
};

}  // namespace tunnel

#endif
