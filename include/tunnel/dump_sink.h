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

#ifndef TUNNEL_DUMP_SINK_H
#define TUNNEL_DUMP_SINK_H

#include <string>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"
#include "tunnel/sink.h"

namespace tunnel {

template <typename T>
class DumpSink : public Sink<T> {
 public:
  explicit DumpSink(const std::string& name = "") : Sink<T>(name) {}

  virtual async_simple::coro::Lazy<void> consume(T &&value) override {
    T v = std::move(value);
    co_return;
  }
};

}  // namespace tunnel

#endif
