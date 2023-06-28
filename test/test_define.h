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

#ifndef TUNNEL_TEST_DEFINE_H
#define TUNNEL_TEST_DEFINE_H

#include <functional>

#include "async_simple/Try.h"
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "tunnel/dispatch.h"
#include "tunnel/pipeline.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

using namespace tunnel;

class SinkTest : public Sink<int> {
 public:
  explicit SinkTest(const std::string& name = "") : Sink<int>(name) {}

  virtual async_simple::coro::Lazy<void> consume(int &&value) override {
    if (callback) {
      callback(value);
    }
    co_return;
  }

  std::function<void(int)> callback;
};

class SourceTest : public Source<int> {
 public:
  explicit SourceTest(int initv = 0, const std::string& name = "") : Source<int>(name), init_value(initv) {}

  virtual async_simple::coro::Lazy<std::optional<int>> generate() override {
    if (num < 100) {
      num = num + 1;
      co_return num + init_value;
    }
    co_return std::optional<int>{};
  }
  int num = 0;
  int init_value = 0;
};

#endif
