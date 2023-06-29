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

#ifndef TUNNEL_PROCESSOR_H
#define TUNNEL_PROCESSOR_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include "tunnel/channel.h"

namespace tunnel {

namespace detail {

static uint64_t GenerateId() {
  static std::atomic<uint64_t> id_{1};
  return id_.fetch_add(1);
}
}  // namespace detail

template <typename T>
class Processor {
 public:
  explicit Processor(const std::string &name = "")
      : processor_id_(detail::GenerateId()), name_(name), input_count_(0) {}

  virtual async_simple::coro::Lazy<void> work() { throw std::runtime_error("work function is not implemented"); }

  async_simple::coro::Lazy<void> work_with_exception() {
    async_simple::Try<void> result = co_await work().coAwaitTry();
    if (result.hasError()) {
      if (!abort_port) {
        std::cerr << "node " << GetId() << " : " << GetName()
                  << " throw exception and abort, we can't handle exception because it doesn't bind abort channel"
                  << std::endl;
        std::abort();
      }
      // We only need to try push because there may be other nodes also writing
      co_await abort_port.GetQueue().TryPush(0);
    }
    co_return;
  }

  // input可能是一写多读或者多写一读的，
  // 如果是多写一读，则需要等待所有写的eof；
  // 如果是一写多读，则需要保证其他读也读到eof；
  async_simple::coro::Lazy<std::optional<T>> Pop(Channel<T> &input, size_t &input_count) {
    while (true) {
      std::optional<T> value;
      if (abort_port) {
        while (true) {
          async_simple::Try<std::optional<T>> v = co_await input.GetQueue().TryPop();
          if (v.available()) {
            value = std::move(v).value();
            break;
          } else {
            async_simple::Try<std::optional<int>> v = co_await abort_port.GetQueue().TryPop();
            if (v.available()) {
              throw std::runtime_error("throw by abort channel");
            }
            co_await async_simple::coro::sleep(std::chrono::milliseconds(20));
          }
        }
      } else {
        value = co_await input.GetQueue().Pop();
      }
      if (value.has_value()) {
        co_return value;
      }
      assert(input_count > 0);
      --input_count;
      if (input_count == 0) {
        co_await Push(std::optional<T>{}, input);
        co_return value;
      }
    }
  }

  async_simple::coro::Lazy<void> Push(std::optional<T> &&v, Channel<T> &output) {
    if (abort_port) {
      while (true) {
        bool succ = co_await output.GetQueue().TryPush(std::move(v));
        if (succ == true) {
          co_return;
        }
        async_simple::Try<std::optional<int>> v = co_await abort_port.GetQueue().TryPop();
        if (v.available()) {
          throw std::runtime_error("throw by abort channel");
        }
        co_await async_simple::coro::sleep(std::chrono::milliseconds(20));
      }
    } else {
      co_await output.GetQueue().Push(std::move(v));
    }
    co_return;
  }

  void SetInputPort(const Channel<T> &input) {
    input_port = input;
    ++input_count_;
  }

  void SetOutputPort(const Channel<T> &output) { output_port = output; }

  void BindAbortChannel(const Channel<int> &abort) { abort_port = abort; }

  virtual ~Processor() {}

  uint64_t &GetId() noexcept { return processor_id_; }

  size_t GetInputCount() const { return input_count_; }

  const Channel<T> &GetInputPort() const { return input_port; }

  Channel<T> &GetInputPort() { return input_port; }

  const Channel<T> &GetOutputPort() const { return output_port; }

  Channel<T> &GetOutputPort() { return output_port; }

  const Channel<T> &GetAbortChannel() const { return abort_port; };

  Channel<T> &GetAbortChannel() { return abort_port; };

  const std::string &GetName() const { return name_; }

 private:
  uint64_t processor_id_;
  std::string name_;
  // This is a channel used to abort execution, set by the pipeline for each node before scheduling.
  // All nodes share the same abort channel. When a node throws an exception, the tunnel will catch the
  // exception and write data to the abort channel before exiting. In addition, all nodes need to wait
  // on both user logic and abort channel through `collectAny` during co_await. If data is returned due to
  // abort channel read something, we should co_return immediately.
  Channel<int> abort_port;

 protected:
  Channel<T> input_port;
  Channel<T> output_port;
  size_t input_count_;
};

template <typename T>
inline void connect(Processor<T> &input, Processor<T> &output) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size));
  input.SetOutputPort(channel);
  output.SetInputPort(channel);
}

template <typename T>
inline void connect(Processor<T> &input, Processor<T> &output, std::shared_ptr<BoundedQueue<std::optional<T>>> &queue) {
  Channel<T> channel(queue);
  input.SetOutputPort(channel);
  output.SetInputPort(channel);
}

}  // namespace tunnel

#endif
