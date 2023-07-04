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
#include "tunnel/one_input_multi_output.h"

namespace tunnel {

/*
 * Dispatch read data from input channel and call the dispatch function to obtain the index value of the output channel
 * to be written. The dispatch function is need to be inherited and implemented by the user.
 */
template <typename T>
class Dispatch : public OneIMultiO<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() {
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
        size_t output_index = dispatch(v.value()) % this->Size();
        Channel<T>& output = this->GetChannel(output_index);
        co_await this->Push(std::move(v), output);
      }
    }
  }

  explicit Dispatch(size_t size, const std::string& name = "dispatch") : OneIMultiO<T>(name, size) {}

  virtual size_t dispatch(const T& value) = 0;
};

}  // namespace tunnel

#endif
