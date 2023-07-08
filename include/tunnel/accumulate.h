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

#ifndef TUNNEL_ACCUMULATE_H
#define TUNNEL_ACCUMULATE_H

#include <string>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"
#include "tunnel/transform.h"

namespace tunnel {

/*
 * Accumulate read data from input channel and call consume() for consumption.
 * After reading EOF, it will call generate() once to get data and write the result to output channel.
 */
template <typename T>
class Accumulate : public Transform<T> {
 public:
  explicit Accumulate(const std::string& name = "accumulate") : Transform<T>(name) {}

 private:
  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T>& input = this->GetInputPort();
    Channel<T>& output = this->GetOutputPort();
    while (true) {
      std::optional<T> v = co_await this->Pop(input, this->input_count_);
      if (v.has_value()) {
        co_await consume(std::move(v).value());
      } else {
        T v = co_await generate();
        co_await this->Push(std::move(v), output);
        co_await this->Push(std::move(std::optional<T>{}), output);
        co_return;
      }
    }
  }

 protected:
  virtual async_simple::coro::Lazy<void> consume(T&& v) = 0;
  virtual async_simple::coro::Lazy<T> generate() = 0;
};

}  // namespace tunnel

#endif
