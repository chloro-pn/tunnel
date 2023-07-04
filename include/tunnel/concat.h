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
#include "tunnel/multi_input_one_output.h"

namespace tunnel {

/*
 * Concat read data from multiple input channels in sequence and write it to output channel.
 * Only after reading EOF from the current input channel will it start reading from the next input channel.
 */
template <typename T>
class Concat : public MultiIOneO<T> {
 public:
  explicit Concat(size_t input_size, const std::string& name = "concat") : MultiIOneO<T>(name, input_size) {}

  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T>& output = this->GetOutputPort();
    for (size_t i = 0; i < this->Size(); ++i) {
      Channel<T>& input = this->GetChannel(i);
      while (true) {
        size_t current_input_count = 1;
        std::optional<T> v = co_await this->Pop(input, current_input_count);
        if (v.has_value()) {
          co_await this->Push(std::move(v), output);
        } else {
          break;
        }
      }
    }
    co_await this->Push(std::optional<T>{}, output);
  }
};

}  // namespace tunnel

#endif
