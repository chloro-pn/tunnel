#ifndef TUNNEL_EXECUTOR_H
#define TUNNEL_EXECUTOR_H

#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <utility>
#include <vector>

#include "async_simple/Executor.h"
#include "executor/timer.h"
#include "rigtorp/MPMCQueue.h"

namespace tunnel {

using async_simple::Executor;

struct TunnelExecutorOption {
  bool force_stop = false;
};

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

  static size_t GetRandomNumber(size_t begin, size_t end) {
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(static_cast<int>(begin), static_cast<int>(end));
    return distribution(generator);
  }

  virtual bool schedule(Func func) override {
    auto id = GetCurrentId();
    size_t worker_index = 0;
    if (id->second == this) {
      // 25%的几率丢给随机的worker处理。
      size_t rn = GetRandomNumber(0, 3);
      if (rn == 0) {
        worker_index = GetRandomNumber(0, GetThreadNum() - 1);
      } else {
        worker_index = id->first;
      }
    } else {
      worker_index = GetRandomNumber(0, GetThreadNum() - 1);
    }
    bool succ = false;
    for (size_t i = 0; i < GetThreadNum(); ++i) {
      size_t try_push_id = (worker_index + i) % GetThreadNum();
      std::unique_lock<std::mutex> guard(*muts_[try_push_id]);
      succ = task_queue_[try_push_id]->try_push(std::move(func));
      if (succ == true) {
        guard.unlock();
        cvs_[try_push_id]->notify_one();
        break;
      }
    }
    return succ;
  }

  virtual bool currentThreadInExecutor() const override { return GetCurrentId()->second == this; }

  size_t GetThreadNum() const noexcept { return thread_num_; }

  bool TryPopStartFrom(size_t index, Func& func) {
    bool succ = false;
    for (size_t i = 0; i < GetThreadNum(); ++i) {
      size_t steal_worker_id = (index + i) % GetThreadNum();
      succ = task_queue_[steal_worker_id]->try_pop(func);
      if (succ == true) {
        break;
      }
    }
    return succ;
  }

  void WakeUpNextWorker(size_t index) {
    size_t next_id = (index + 1) % GetThreadNum();
    cvs_[next_id]->notify_one();
  }

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
      bool succ = TryPopStartFrom(worker_id, func);
      if (succ == true) {
        WakeUpNextWorker(worker_id);
        func();
        continue;
      } else {
        if (stop_when_try_pop_failed == true) {
          return;
        }
        std::unique_lock lock(*muts_[worker_id]);
        cvs_[worker_id]->wait(lock, [&]() { return this->stop_ == true || TryPopStartFrom(worker_id, func); });
        if (stop_ == true) {
          stop_when_try_pop_failed = true;
        } else {
          lock.unlock();
          func();
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
