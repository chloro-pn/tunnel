#ifndef TUNNEL_EXECUTOR_H
#define TUNNEL_EXECUTOR_H

#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "async_simple/Executor.h"
#include "executor/timer.h"
#include "rigtorp/MPMCQueue.h"

namespace tunnel {

using async_simple::Executor;

class TunnelExecutor : public Executor {
 public:
  using Executor::Func;

  std::pair<size_t, TunnelExecutor*>* GetCurrentId() const {
    static thread_local std::pair<size_t, TunnelExecutor*> id{-1, nullptr};
    return &id;
  }

  TunnelExecutor(size_t thread_num, const std::string& name = "default")
      : Executor(name), stop_(false), thread_num_(thread_num), timer_manager_(this) {
    assert(thread_num > 0);
    for (size_t i = 0; i < thread_num_; ++i) {
      task_queue_.emplace_back(new rigtorp::MPMCQueue<Func>{102400});
      muts_.emplace_back(new std::mutex{});
      cvs_.emplace_back(new std::condition_variable{});
    }
    // wait all worker init finished.
    std::mutex init_mut;
    std::condition_variable init_cv;
    size_t finished = 0;
    timer_manager_.Run(init_cv, init_mut, finished);
    for (size_t i = 0; i < thread_num_; ++i) {
      workers_.emplace_back(
          [this, i, &init_mut, &init_cv, &finished]() { this->WorkerMain(i, init_mut, init_cv, finished); });
    }
    std::unique_lock<std::mutex> guard(init_mut);
    init_cv.wait(guard, [&]() { return finished == thread_num_ + 1; });
  }

  virtual bool schedule(Func func) override {
    auto id = GetCurrentId();
    size_t worker_index = 0;
    if (id->second == this) {
      worker_index = id->first;
    } else {
      worker_index = rand() % GetThreadNum();
    }
    bool succ = false;
    for (size_t i = 0; i < GetThreadNum(); ++i) {
      size_t try_push_id = (worker_index + i) % GetThreadNum();
      succ = task_queue_[try_push_id]->try_push(std::move(func));
      if (succ == true) {
        cvs_[try_push_id]->notify_one();
        break;
      }
    }
    return succ;
  }

  virtual bool currentThreadInExecutor() const override { return GetCurrentId()->second == this; }

  size_t GetThreadNum() const noexcept { return thread_num_; }

  void WorkerMain(size_t worker_id, std::mutex& init_mut, std::condition_variable& init_cv, size_t& finished) {
    auto id = GetCurrentId();
    id->first = worker_id;
    id->second = this;
    std::unique_lock<std::mutex> guard(init_mut);
    finished += 1;
    guard.unlock();
    init_cv.notify_one();
    bool stop_when_try_pop_failed = false;
    while (true) {
      Func func;
      bool succ = false;
      for (size_t i = 0; i < GetThreadNum(); ++i) {
        size_t steal_worker_id = (worker_id + i) % GetThreadNum();
        succ = task_queue_[steal_worker_id]->try_pop(func);
        if (succ == true) {
          break;
        }
      }
      if (succ == true) {
        func();
        continue;
      } else {
        if (stop_when_try_pop_failed == true) {
          return;
        }
        std::unique_lock lock(*muts_[worker_id]);
        cvs_[worker_id]->wait_for(lock, std::chrono::milliseconds(10), [&]() { return this->stop_ == true; });
        if (stop_ == true) {
          stop_when_try_pop_failed = true;
        }
      }
    }
  }

  void Stop() {
    timer_manager_.Stop();
    for (size_t i = 0; i < GetThreadNum(); ++i) {
      muts_[i]->lock();
    }
    stop_ = true;
    for (size_t i = 0; i < GetThreadNum(); ++i) {
      muts_[i]->unlock();
      cvs_[i]->notify_one();
    }
    for (auto& work : workers_) {
      work.join();
    }
  }

  ~TunnelExecutor() noexcept {
    if (stop_ == false) {
      Stop();
    }
  }

  // for test.
  void schedule_timer(Func func, Duration dur) { schedule(std::move(func), std::move(dur)); }

 protected:
  virtual void schedule(Func func, Duration dur) override { timer_manager_.PushTimer(std::move(func), dur); }

 private:
  bool stop_;
  size_t thread_num_;
  TimerManager timer_manager_;
  std::vector<std::thread> workers_;
  std::vector<std::unique_ptr<rigtorp::MPMCQueue<Func>>> task_queue_;
  std::vector<std::unique_ptr<std::mutex>> muts_;
  std::vector<std::unique_ptr<std::condition_variable>> cvs_;
};

inline void TimerManager::PushTimer(Func func, Duration duration) {
  auto timer = std::make_shared<asio::steady_timer>(ctx_, duration);
  timer->async_wait([this, func = std::move(func), self = timer](asio::error_code ec) {
    if (ec) {
      throw std::runtime_error(ec.message());
    }
    bool succ = executor_->schedule(std::move(func));
    if (succ == false) {
      throw std::runtime_error("schedule timer failed");
    }
  });
}

}  // namespace tunnel

#endif
