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

#ifndef TUNNEL_SINK_H
#define TUNNEL_SINK_H

#include <memory>
#include <string>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Sink : public Processor<T> {
 public:
  explicit Sink(const std::string& name = "sink") : Processor<T>(name) {}

 private:
  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T>& input = this->GetInputPort();
    while (true) {
      std::optional<T> value = co_await this->Pop(input, this->input_count_);
      if (value.has_value()) {
        co_await consume(std::move(value).value());
      } else {
        co_return;
      }
    }
  }

  virtual async_simple::coro::Lazy<void> hosted_mode() override {
    Channel<T>& input = this->GetInputPort();
    co_await this->close_input(input, this->input_count_);
    co_return;
  }

 protected:
  virtual async_simple::coro::Lazy<void> consume(T&& value) = 0;
};

}  // namespace tunnel

#endif
