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
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include "async_simple/coro/Lazy.h"
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
      : processor_id_(detail::GenerateId()), name_(name), input_count_(0), output_count_(0) {}

  virtual async_simple::coro::Lazy<void> work() { throw std::runtime_error("work function is not implemented"); }

  async_simple::coro::Lazy<void> work_with_exception() {
    async_simple::Try<void> result = co_await work().coAwaitTry();
    if (result.hasError()) {
      std::cerr << "node " << GetId() << " : " << GetName()
                << " throw exception and abort, we can't handle exception now" << std::endl;
      std::abort();
    }
  }

  // input可能是一写多读或者多写一读的，
  // 如果是多写一读，则需要等待所有写的eof；
  // 如果是一写多读，则需要保证其他读也读到eof；
  async_simple::coro::Lazy<std::optional<T>> Pop() {
    while (true) {
      std::optional<T> value = co_await input_port.GetQueue().Pop();
      if (value.has_value()) {
        co_return value;
      }
      assert(input_count_ > 0);
      --input_count_;
      if (input_count_ == 0) {
        co_await input_port.GetQueue().Push(std::optional<T>{});
        co_return value;
      }
    }
  }

  void SetInputPort(const Channel<T> &input) {
    input_port = input;
    ++input_count_;
  }

  void SetOutputPort(const Channel<T> &output) {
    output_port = output;
    ++output_count_;
  }

  virtual ~Processor() {}

  uint64_t &GetId() noexcept { return processor_id_; }

  const Channel<T> &GetInputPort() const { return input_port; }

  Channel<T> &GetInputPort() { return input_port; }

  const Channel<T> &GetOutputPort() const { return output_port; }

  Channel<T> &GetOutputPort() { return output_port; }

  const std::string &GetName() const { return name_; }

 private:
  uint64_t processor_id_;
  std::string name_;
  Channel<T> input_port;
  Channel<T> output_port;

 protected:
  size_t input_count_;
  size_t output_count_;
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
