#ifndef TUNNEL_TIMER_H
#define TUNNEL_TIMER_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "asio/executor_work_guard.hpp"
#include "asio/io_context.hpp"
#include "asio/steady_timer.hpp"

namespace tunnel {

class TunnelExecutor;

class TimerManager {
 public:
  using Func = std::function<void()>;
  using Duration = std::chrono::duration<int64_t, std::micro>;

  explicit TimerManager(TunnelExecutor* executor)
      : executor_(executor), start_(false), stop_(false), ctx_(), work(asio::make_work_guard(ctx_)) {}

  void Run(std::condition_variable& init_cv, std::mutex& init_mut, size_t& init_count) {
    if (start_ == false) {
      timer_work_ = std::thread([&]() {
        std::unique_lock<std::mutex> guard(init_mut);
        init_count += 1;
        init_cv.notify_one();
        guard.unlock();
        ctx_.run();
      });
      start_ = true;
    }
  }

  void PushTimer(Func func, Duration duration);

  void Stop() {
    if (start_ == true && stop_ == false) {
      work.reset();
      timer_work_.join();
      stop_ = true;
    }
  }

  ~TimerManager() { Stop(); }

 private:
  TunnelExecutor* executor_;
  std::thread timer_work_;
  bool start_;
  bool stop_;
  asio::io_context ctx_;
  asio::executor_work_guard<asio::io_context::executor_type> work;
};

}  // namespace tunnel

#endif
