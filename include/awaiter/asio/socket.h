#ifndef TUNNEL_AWAITER_SOCKET_H
#define TUNNEL_AWAITER_SOCKET_H

#include <stdexcept>
#include <string>

#include "asio.hpp"
#include "async_simple/experimental/coroutine.h"

namespace tunnel {

namespace ip::tcp {

class SocketAwaiterBase {
 public:
  SocketAwaiterBase() : succ_(true), err_msg_() {}

  void handle_error(const asio::error_code& ec) {
    if (ec) {
      succ_ = false;
      err_msg_ = ec.message();
    }
  }

  template <typename T>
  T await_resume_or_throw(T&& v) {
    if (succ_ == false) {
      throw std::runtime_error(std::move(err_msg_));
    }
    return std::forward<T>(v);
  }

  void await_resume_or_throw() {
    if (succ_ == false) {
      throw std::runtime_error(std::move(err_msg_));
    }
  }

 protected:
  bool succ_;
  std::string err_msg_;
};

class SocketConnectAwaiter : public SocketAwaiterBase {
 public:
  SocketConnectAwaiter(asio::ip::tcp::socket& socket, const asio::ip::tcp::endpoint& ep) : socket_(socket), ep_(ep) {}

  void await_suspend(std::coroutine_handle<> h) {
    socket_.async_connect(ep_, [this, h](const asio::error_code& ec) mutable {
      if (ec) {
        handle_error(ec);
      }
      h.resume();
    });
  }

  bool await_ready() const noexcept { return false; }

  bool await_resume() { return succ_; }

 private:
  asio::ip::tcp::socket& socket_;
  const asio::ip::tcp::endpoint ep_;
};

template <typename MutableBufferSequence>
class SocketReadAwaiter : public SocketAwaiterBase {
 public:
  SocketReadAwaiter(asio::ip::tcp::socket& socket, const MutableBufferSequence& buf, bool throw_eof = true)
      : socket_(socket), buf_(buf), transferred_(0), throw_eof_(throw_eof) {}

  bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    asio::async_read(socket_, buf_, [this, h](asio::error_code ec, size_t transferred) mutable {
      if (ec && ec != asio::error::eof) {
        handle_error(ec);
      } else if (ec && ec == asio::error::eof && throw_eof_ == true) {
        handle_error(ec);
      } else {
        transferred_ = transferred;
      }
      h.resume();
    });
  }

  size_t await_resume() { return await_resume_or_throw(transferred_); }

 private:
  asio::ip::tcp::socket& socket_;
  const MutableBufferSequence& buf_;
  size_t transferred_;
  bool throw_eof_;
};

template <typename ConstBufferSequence>
class SocketWriteAwaiter : public SocketAwaiterBase {
 public:
  SocketWriteAwaiter(asio::ip::tcp::socket& socket, const ConstBufferSequence& buf)
      : socket_(socket), buf_(buf), transferred_(0) {}

  bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    asio::async_write(socket_, buf_, [this, h](asio::error_code ec, size_t transferred) mutable {
      if (ec) {
        handle_error(ec);
      } else {
        transferred_ = transferred;
      }
      h.resume();
    });
  }

  size_t await_resume() { return await_resume_or_throw(transferred_); }

 private:
  asio::ip::tcp::socket& socket_;
  const ConstBufferSequence& buf_;
  size_t transferred_;
};

class SocketAcceptAwaiter : public SocketAwaiterBase {
 public:
  SocketAcceptAwaiter(asio::ip::tcp::acceptor& acceptor) : acceptor_(acceptor), socket_(acceptor_.get_executor()) {}

  bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    acceptor_.async_accept([this, h](asio::error_code ec, asio::ip::tcp::socket socket) mutable {
      if (ec) {
        handle_error(ec);
      } else {
        socket_ = std::move(socket);
      }
      h.resume();
    });
  }

  asio::ip::tcp::socket await_resume() { return await_resume_or_throw(std::move(socket_)); }

 private:
  asio::ip::tcp::acceptor& acceptor_;
  asio::ip::tcp::socket socket_;
};

}  // namespace ip::tcp

}  // namespace tunnel

#endif
