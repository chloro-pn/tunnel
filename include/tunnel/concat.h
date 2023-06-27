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

#ifndef TUNNEL_CONCAT_H
#define TUNNEL_CONCAT_H

#include <cassert>
#include <string>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Concat : public Processor<T> {
 public:
  explicit Concat(const std::string& name = "") : Processor<T>(name) {}

  /*
   * 由pipeline接口保证，concat的input不存在一写多读和多写一读的情况，因此不需要记录
   * input_count_。
   */
  void AddInputPort(const Channel<T>& channel) { inputs_.push_back(channel); }

  size_t Size() const { return inputs_.size(); }

  virtual async_simple::coro::Lazy<void> work() override {
    assert(!inputs_.empty());
    Channel<T>& output = this->GetOutputPort();
    for (auto it = inputs_.begin(); it != inputs_.end(); ++it) {
      Channel<T>& input = *it;
      while (true) {
        std::optional<T> v = co_await input.GetQueue().Pop();
        if (v.has_value()) {
          co_await output.GetQueue().Push(std::move(v));
        } else {
          break;
        }
      }
    }
    co_await output.GetQueue().Push(std::optional<T>{});
  }

 private:
  std::vector<Channel<T>> inputs_;
};

template <typename T>
inline void connect(Processor<T>& input, Concat<T>& output) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size));
  input.SetOutputPort(channel);
  output.AddInputPort(channel);
}

}  // namespace tunnel

#endif
