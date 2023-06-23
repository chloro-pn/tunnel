#pragma once

#include "async_simple/Try.h"
#include "async_simple/coro/ConditionVariable.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"

#include <concepts>
#include <deque>
#include <type_traits>

namespace tunnel {

using async_simple::Try;
using async_simple::coro::ConditionVariable;
using async_simple::coro::Lazy;

template <typename T, class Lock = async_simple::coro::Mutex>
class BoundedQueue {
public:
  using element_type = T;

  explicit BoundedQueue(size_t capacity) : capacity_(capacity) {}

  Lazy<element_type> Pop() {
    auto lock = co_await mut_.coScopedLock();
    if (queue_.empty()) {
      co_await empty_cv_.wait(mut_, [&]() { return !this->queue_.empty(); });
    }
    element_type result = std::move(queue_.front());
    queue_.pop_front();
    filled_cv_.notifyOne();
    co_return result;
  }

  Lazy<Try<element_type>> TryPop() {
    auto lock = co_await mut_.coScopedLock();
    if (queue_.empty()) {
      co_return Try<element_type>{};
    }
    element_type result = std::move(queue_.front());
    queue_.pop_front();
    filled_cv_.notifyOne();
    co_return Try<element_type>(std::move(result));
  }

  template <typename T2>
  requires std::is_convertible_v<T2, element_type> Lazy<void>
  Push(T2 &&element) {
    auto lock = co_await mut_.coScopedLock();
    // 逻辑上来说不需要这个分支判断，但是这样处理可以避免
    // 一次co_await Lazy
    if (queue_.size() == this->Capacity()) {
      co_await filled_cv_.wait(
          mut_, [&]() { return this->queue_.size() < this->Capacity(); });
    }
    queue_.emplace_back(std::forward<T2>(element));
    lock.unlock();
    empty_cv_.notifyOne();
  }

  template <typename T2>
  requires std::is_convertible_v<T2, element_type> Lazy<bool>
  TryPush(T2 &&element) {
    auto lock = co_await mut_.coScopedLock();
    if (queue_.size() == Capacity()) {
      co_return false;
    }
    queue_.emplace_back(std::forward<T2>(element));
    lock.unlock();
    empty_cv_.notifyOne();
    co_return true;
  }

  size_t Capacity() const noexcept { return capacity_; }

private:
  Lock mut_;
  ConditionVariable<Lock> empty_cv_;
  ConditionVariable<Lock> filled_cv_;
  std::deque<element_type> queue_;
  const size_t capacity_;
};

} // namespace tunnel