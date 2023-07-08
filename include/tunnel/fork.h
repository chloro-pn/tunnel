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
#include <string>
#include <vector>

#include "tunnel/channel.h"
#include "tunnel/one_input_multi_output.h"

namespace tunnel {

/*
 * Fork read data from input channel, copy it and write it to each output channel.
 */
template <typename T>
class Fork : public OneIMultiO<T> {
 public:
  explicit Fork(size_t size, const std::string& name = "fork")
      : OneIMultiO<T>(name, size), copy_([](const T& v) { return v; }) {}

 private:
  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T>& input = this->GetInputPort();
    while (true) {
      std::optional<T> v = co_await this->Pop(input, this->input_count_);
      if (!v.has_value()) {
        for (size_t index = 0; index < this->Size(); ++index) {
          Channel<T>& output = this->GetChannel(index);
          co_await this->Push(std::optional<T>{}, output);
        }
        co_return;
      } else {
        for (size_t index = 0; index < this->Size(); ++index) {
          Channel<T>& output = this->GetChannel(index);
          if (index == this->Size() - 1) {
            co_await this->Push(std::move(v), output);
          } else {
            T new_v = copy_(v.value());
            co_await this->Push(std::move(new_v), output);
          }
        }
      }
    }
  }

 private:
  std::function<T(const T&)> copy_;
};

}  // namespace tunnel

#endif
