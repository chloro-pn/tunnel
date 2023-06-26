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

#ifndef TUNNEL_FORK_H
#define TUNNEL_FORK_H

#include <cassert>
#include <functional>
#include <vector>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Fork : public Processor<T> {
 public:
  explicit Fork(size_t size) : size_(size), copy_([](const T& v) { return v; }) {}

  void AddOutput(const Channel<T>& channel) {
    if (outputs_.size() < size_) {
      outputs_.push_back(channel);
    }
  }

  size_t GetSize() const { return size_; }

  virtual async_simple::coro::Lazy<void> work() override {
    assert(outputs_.size() > 0);
    while (true) {
      std::optional<T> v = co_await this->Pop();
      if (!v.has_value()) {
        for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
          co_await (*it).GetQueue().Push(std::optional<T>{});
        }
        co_return;
      } else {
        for (size_t index = 0; index < outputs_.size(); ++index) {
          if (index == outputs_.size() - 1) {
            co_await outputs_[index].GetQueue().Push(std::move(v));
          } else {
            T new_v = copy_(v.value());
            co_await outputs_[index].GetQueue().Push(std::move(new_v));
          }
        }
      }
    }
  }

 private:
  size_t size_;
  std::vector<Channel<T>> outputs_;
  std::function<T(const T&)> copy_;
};

template <typename T>
inline void connect(Fork<T>& input, Processor<T>& output) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size));
  input.AddOutput(channel);
  output.SetInputPort(channel);
}

}  // namespace tunnel

#endif
