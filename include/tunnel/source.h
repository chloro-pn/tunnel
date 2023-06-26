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

#ifndef TUNNEL_SOURCE_H
#define TUNNEL_SOURCE_H

#include <memory>
#include <optional>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Source : public Processor<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T> &output = this->GetOutputPort();
    while (true) {
      std::optional<T> v = co_await generate();
      bool read_eof = !v.has_value();
      co_await output.GetQueue().Push(std::move(v));
      if (read_eof) {
        co_return;
      }
    }
  }

  virtual async_simple::coro::Lazy<std::optional<T>> generate() = 0;
};

}  // namespace tunnel

#endif
