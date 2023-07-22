#ifndef TUNNEL_NET_PROXY_H
#define TUNNEL_NET_PROXY_H

#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "asio/asio.hpp"
#include "async_simple/coro/ConditionVariable.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "async_simple/coro/Sleep.h"
#include "awaiter/asio/socket.h"
#include "executor/tunnel_executor.h"
#include "tunnel/channel.h"
#include "tunnel/event_count.h"
namespace tunnel {

inline void AbortIfError(async_simple::Try<void> r) {
  if (r.hasError()) {
    try {
      std::rethrow_exception(r.getException());
    } catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
      std::abort();
    }
  }
}

/*
 * 单例模式，进程内唯一
 * 用来与其他计算节点进行网络数据传输
 */
class NetProxy {
  using socket = asio::ip::tcp::socket;
  using acceptor = asio::ip::tcp::acceptor;
  using endpoint = asio::ip::tcp::endpoint;

 public:
  static NetProxy& Instance() {
    static NetProxy obj;
    return obj;
  }

  void Init(const std::vector<std::pair<std::string, uint16_t>>& workers, size_t myself) {
    workers_ = workers;
    myself_ = myself;
    init_ = true;
  }

  void Start() {
    assert(init_ == true);
    assert(workers_.size() > 1);
    assert(myself_ >= 0 && myself_ < workers_.size());
    size_t total_pop_sockets = workers_.size() - 1;
    EventCount ec(2 * (workers_.size() - 1));
    using namespace tunnel::ip::tcp;
    auto accept_task = [this, total_pop_sockets, &ec]() -> async_simple::coro::Lazy<void> {
      size_t count = 0;
      while (true) {
        auto socket = co_await SocketAcceptAwaiter(this->acceptor_);
        this->pop_sockets_.emplace_back(std::move(socket));
        count += 1;
        ec.Succ();
        if (total_pop_sockets == count) {
          co_return;
        }
      }
    };

    auto conn_task = [this, &ec]() -> async_simple::coro::Lazy<void> {
      for (size_t i = 0; i < this->workers_.size(); ++i) {
        if (i == this->myself_) {
          // 占位，使得下标与id一致
          this->push_sockets_.emplace_back(this->ctx_);
          continue;
        }
        auto& ip = this->workers_[i].first;
        auto& port = this->workers_[i].second;
        size_t retry_count = 0;
        using namespace std::chrono_literals;
        std::array<std::chrono::milliseconds, 5> wait_ms{10ms, 200ms, 1000ms, 5000ms, 30000ms};
        while (retry_count < 5) {
          socket conn(this->ctx_);
          bool succ = co_await SocketConnectAwaiter(conn, {asio::ip::address::from_string(ip), port});
          if (succ) {
            this->push_sockets_.emplace_back(std::move(conn));
            ec.Succ();
            co_return;
          }
          co_await async_simple::coro::sleep(wait_ms[retry_count]);
          retry_count += 1;
        }
        // failed
        ec.Fail();
      }
    };

    accept_task().via(&ex_).start([](async_simple::Try<void> r) { AbortIfError(std::move(r)); });

    conn_task().via(&ex_).start([](async_simple::Try<void> r) { AbortIfError(std::move(r)); });

    ec.Wait();
    if (ec.allSucc() == false) {
      std::cerr << "network error" << std::endl;
      std::abort();
    }
  }

  void Stop();

  template <typename T>
  async_simple::coro::Lazy<void> Push(uint64_t id, const std::string& topic, std::optional<T>&& value) {
    block b;
    if (value.has_value()) {
      b.bin_data = T::serialize(std::move(value).value());
      b.eof = false;
    } else {
      b.eof = true;
    }
    auto guard = co_await push_mutex_.coScopedLock();
    push_blocks_[id][topic].push_back(std::move(b));
    push_cv_.notifyAll();
    co_return;
  }

  template <typename T>
  async_simple::coro::Lazy<std::optional<T>> Pop(const std::string& topic) {
    co_await pop_mutex_.coLock();
    pop_cv_.wait(pop_mutex_, [&]() { return pop_blocks_[topic].empty() == false; });
    block b = std::move(pop_blocks_[topic].front());
    pop_blocks_[topic].pop_front();
    pop_mutex_.unlock();
    if (b.eof == true) {
      co_return std::optional<T>{};
    }
    T v = T::descriable(std::move(b.bin_data));
    co_return std::optional<T>(v);
  }

 private:
  NetProxy() : init_(false), ctx_(1), guard_(asio::make_work_guard(ctx_)), ex_(1), acceptor_(ctx_) {}

  struct block {
    std::string bin_data;
    bool eof;
  };

  async_simple::coro::Mutex push_mutex_;
  async_simple::coro::ConditionVariable<async_simple::coro::Mutex> push_cv_;
  async_simple::coro::Mutex pop_mutex_;
  async_simple::coro::ConditionVariable<async_simple::coro::Mutex> pop_cv_;
  std::unordered_map<uint64_t, std::unordered_map<std::string, std::vector<block>>> push_blocks_;
  std::unordered_map<std::string, block> pop_blocks_;

  std::vector<std::pair<std::string, uint16_t>> workers_;
  size_t myself_;
  bool init_;

  asio::io_context ctx_;
  asio::executor_work_guard<asio::io_context::executor_type> guard_;
  std::thread worker_;
  TunnelExecutor ex_;

  acceptor acceptor_;
  std::vector<std::unique_ptr<socket>> push_sockets_;
  std::vector<std::unique_ptr<socket>> pop_sockets_;
};

}  // namespace tunnel

#endif
