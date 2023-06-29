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

#ifndef TUNNEL_DISPATCH_H
#define TUNNEL_DISPATCH_H

#include <stdexcept>
#include <string>
#include <vector>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Dispatch : public Processor<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() {
    Channel<T>& input = this->GetInputPort();
    while (true) {
      std::optional<T> v = co_await this->Pop(input, this->input_count_);
      if (!v.has_value()) {
        for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
          Channel<T>& output = (*it);
          co_await this->Push(std::optional<T>{}, output);
        }
        co_return;
      } else {
        size_t output_index = dispatch(v.value()) % outputs_.size();
        co_await this->Push(std::move(v), outputs_[output_index]);
      }
    }
  }

  explicit Dispatch(size_t size, const std::string& name = "") : Processor<T>(name), size_(size) {}

  void AddOutput(const Channel<T>& channel) {
    if (outputs_.size() < size_) {
      outputs_.push_back(channel);
    }
  }

  virtual size_t dispatch(const T& value) = 0;

  size_t GetSize() const { return size_; }

 private:
  const size_t size_;
  std::vector<Channel<T>> outputs_;
};

template <typename T>
inline void connect(Dispatch<T>& input, Processor<T>& output) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size));
  input.AddOutput(channel);
  output.SetInputPort(channel);
}

}  // namespace tunnel

#endif
