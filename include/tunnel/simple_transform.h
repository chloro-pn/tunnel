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

#ifndef TUNNEL_SIMPLE_TRANSFORM_H
#define TUNNEL_SIMPLE_TRANSFORM_H

#include <cassert>
#include <memory>
#include <optional>
#include <string>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"
#include "tunnel/transform.h"

namespace tunnel {

template <typename T>
class SimpleTransform : public Transform<T> {
 public:
  explicit SimpleTransform(const std::string &name = "simple_transform") : Transform<T>(name) {}

  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T> &input = this->GetInputPort();
    Channel<T> &output = this->GetOutputPort();
    while (true) {
      std::optional<T> v = co_await this->Pop(input, this->input_count_);
      if (v.has_value()) {
        T new_v = co_await transform(std::move(v).value());
        co_await this->Push(std::move(new_v), output);
      } else {
        co_await this->Push(std::move(v), output);
        co_return;
      }
    }
  }

  virtual async_simple::coro::Lazy<T> transform(T &&value) = 0;
};

template <typename T>
class NoOpTransform : public SimpleTransform<T> {
 public:
  static std::unique_ptr<Processor<T>> MergeSinkAndSource(std::unique_ptr<Processor<T>> &&sink,
                                                          std::unique_ptr<Processor<T>> &&source) {
    auto no_op = std::make_unique<NoOpTransform<T>>();
    no_op->input_count_ = sink->GetInputCount();
    no_op->input_port = sink->GetInputPort();
    no_op->output_port = source->GetOutputPort();
    no_op->BindAbortChannel(sink->GetAbortChannel());
    return no_op;
  }

  NoOpTransform() : SimpleTransform<T>("no_op") {}

  virtual async_simple::coro::Lazy<T> transform(T &&value) override { co_return std::move(value); }
};

}  // namespace tunnel

#endif
