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

#ifndef TUNNEL_FILTER_H
#define TUNNEL_FILTER_H

#include <optional>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"
#include "tunnel/transform.h"

namespace tunnel {

template <typename T>
class Filter : public Transform<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() {
    Channel<T> &output = this->GetOutputPort();
    while (true) {
      std::optional<T> v = co_await this->Pop();
      if (v.has_value()) {
        bool need_filter = filter(v.value());
        if (!need_filter) {
          co_await output.GetQueue().Push(std::move(v));
        }
      } else {
        co_await output.GetQueue().Push(std::move(v));
        co_return;
      }
    }
  }

  virtual bool filter(const T &v) = 0;
};

}  // namespace tunnel

#endif