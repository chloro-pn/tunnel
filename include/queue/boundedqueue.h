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

#ifndef TUNNEL_BOUNDED_QUEUE_H
#define TUNNEL_BOUNDED_QUEUE_H

#include <concepts>
#include <deque>

#include "async_simple/coro/ConditionVariable.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"

namespace tunnel {

using async_simple::Try;
using async_simple::coro::ConditionVariable;
using async_simple::coro::Lazy;

/*
 * BoundedQueue is a Thread safety bounded queue through which multiple Processors can transfer information.
 * It does not block threads and suspends coroutines when necessary.
 */
template <typename T, class Lock = async_simple::coro::Mutex>
class BoundedQueue {
 public:
  using element_type = T;

  explicit BoundedQueue(size_t capacity) : capacity_(capacity) {}

  Lazy<element_type> Pop() noexcept {
    auto lock = co_await mut_.coScopedLock();
    if (queue_.empty()) {
      co_await empty_cv_.wait(mut_, [&]() { return !this->queue_.empty(); });
    }
    element_type result = std::move(queue_.front());
    queue_.pop_front();
    filled_cv_.notifyOne();
    co_return result;
  }

  Lazy<Try<element_type>> TryPop() noexcept {
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
  Lazy<void> Push(T2 &&element) noexcept {
    auto lock = co_await mut_.coScopedLock();
    if (queue_.size() == this->Capacity()) {
      co_await filled_cv_.wait(mut_, [&]() { return this->queue_.size() < this->Capacity(); });
    }
    queue_.emplace_back(std::forward<T2>(element));
    empty_cv_.notifyOne();
  }

  template <typename T2>
  Lazy<bool> TryPush(T2 &&element) noexcept {
    auto lock = co_await mut_.coScopedLock();
    if (queue_.size() == Capacity()) {
      co_return false;
    }
    queue_.emplace_back(std::forward<T2>(element));
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

}  // namespace tunnel

#endif
