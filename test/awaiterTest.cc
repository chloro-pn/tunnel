/*
 * Copyright 2023, chloro-pn;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <thread>

#include "asio/executor_work_guard.hpp"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "awaiter/asio/socket.h"
#include "executor/tunnel_executor.h"
#include "gtest/gtest.h"
#include "test_define.h"

using namespace tunnel;
using namespace async_simple::coro;

struct IoContextRunner {
  asio::io_context ctx_;
  asio::executor_work_guard<asio::io_context::executor_type> guard_;
  std::thread worker_;

  IoContextRunner() : guard_(asio::make_work_guard(ctx_)) {
    worker_ = std::thread([&] {
      try {
        this->ctx_.run();
      } catch (const std::exception& e) {
        std::printf("io_context exception : %s\n", e.what());
      }
    });
  }

  ~IoContextRunner() {
    guard_.reset();
    ctx_.stop();
    worker_.join();
  }
};

class EchoServer {
 public:
  explicit EchoServer(asio::io_context& ctx) : ctx_(ctx) { asio::co_spawn(ctx_, listener(), asio::detached); }

  asio::awaitable<void> echo(asio::ip::tcp::socket socket) {
    try {
      char data[1024];
      for (;;) {
        std::size_t n = co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
        co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
      }
    } catch (std::exception& e) {
      std::printf("echo Exception: %s\n", e.what());
    }
  }

  asio::awaitable<void> listener() {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, {asio::ip::address::from_string("127.0.0.1"), 12345});
    for (;;) {
      asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
      asio::co_spawn(executor, echo(std::move(socket)), asio::detached);
    }
  }

 private:
  asio::io_context& ctx_;
};

TEST(awaiterTest, basic) {
  IoContextRunner io;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EchoServer echo(io.ctx_);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  size_t read_n = 0;
  std::string read_msg;
  auto task = [&]() -> Lazy<> {
    asio::ip::tcp::socket socket(io.ctx_);
    co_await ip::tcp::SocketConnectAwaiter(socket,
                                           asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 12345));

    std::string buffer(1024, 'a');
    std::string buffer2(1024, 'b');
    co_await ip::tcp::SocketWriteAwaiter(socket, asio::buffer(buffer));
    size_t transferred = co_await ip::tcp::SocketReadAwaiter(socket, asio::buffer(buffer2));
    read_n = transferred;
    read_msg = buffer2;
    co_return;
  };
  syncAwait(task());
  EXPECT_EQ(read_n, 1024);
  EXPECT_EQ(read_msg, std::string(1024, 'a'));

  auto task2 = [&]() -> Lazy<> {
    asio::ip::tcp::socket socket(io.ctx_);
    co_await ip::tcp::SocketConnectAwaiter(socket,
                                           asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 12346));
  };
  // connection refused exception
  EXPECT_THROW(syncAwait(task2()), std::runtime_error);
  std::string read_msg2;
  auto accept_task = [&]() -> Lazy<> {
    asio::ip::tcp::acceptor accept(io.ctx_,
                                   asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 12346));
    asio::ip::tcp::socket conn = co_await ip::tcp::SocketAcceptAwaiter(accept);
    std::string buffer(1024, 'c');
    co_await ip::tcp::SocketWriteAwaiter(conn, asio::buffer(buffer));
    std::string buffer2(1024, '\0');
    co_await ip::tcp::SocketReadAwaiter(conn, asio::buffer(buffer2));
    read_msg2 = buffer2;
    co_return;
  };

  auto client_task = [&]() -> Lazy<> {
    asio::ip::tcp::socket socket(io.ctx_);
    co_await ip::tcp::SocketConnectAwaiter(socket,
                                           asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 12346));
    std::string buffer(1024, 'a');
    co_await ip::tcp::SocketReadAwaiter(socket, asio::buffer(buffer));
    co_await ip::tcp::SocketWriteAwaiter(socket, asio::buffer(buffer));
    co_return;
  };
  tunnel::TunnelExecutor ex(2);
  EventCount ec(2);
  accept_task().via(&ex).start([&](auto&&) { ec.Notify(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  client_task().via(&ex).start([&](auto&&) { ec.Notify(); });
  ec.Wait();
  EXPECT_EQ(read_msg2, std::string(1024, 'c'));
}
