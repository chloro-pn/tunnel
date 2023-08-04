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

#define protected public
#define private public

#include <condition_variable>
#include <mutex>

#include "asio.hpp"
#include "async_simple/Try.h"
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "tunnel/dispatch.h"
#include "tunnel/event_count.h"
#include "tunnel/pipeline.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

using namespace tunnel;

namespace tunnel {

template <>
inline size_t GetBytes(const int& v) {
  return sizeof(v);
}
}  // namespace tunnel

template <typename T = int>
class SinkTest : public Sink<T> {
 public:
  explicit SinkTest(const std::string& name = "") : Sink<T>(name) {}

  virtual async_simple::coro::Lazy<void> consume(T&& value) override {
    if (callback) {
      callback(value);
    }
    co_return;
  }

  std::function<void(T)> callback;
};

class ThrowSinkTest : public Sink<int> {
 public:
  explicit ThrowSinkTest(const std::string& name = "") : Sink<int>(name) {}

  virtual async_simple::coro::Lazy<void> consume(int&& value) override {
    throw std::runtime_error("throw sink test");
    co_return;
  }
};

template <typename T = int, size_t generate_count = 100>
class SourceTest : public Source<T> {
 public:
  explicit SourceTest(int initv = 0, const std::string& name = "") : Source<T>(name), init_value(initv) {}

  virtual async_simple::coro::Lazy<std::optional<T>> generate() override {
    if (static_cast<size_t>(num) < generate_count) {
      num = num + 1;
      co_return T(num + init_value);
    }
    co_return std::optional<T>{};
  }
  int num = 0;
  int init_value = 0;
};

struct IoContextRunner {
  asio::io_context ctx_;
  asio::executor_work_guard<asio::io_context::executor_type> guard_;
  std::thread worker_;

  IoContextRunner() : guard_(asio::make_work_guard(ctx_)) {
    worker_ = std::thread([&] {
      try {
        this->ctx_.run();
      } catch (const std::exception& e) {
        std::printf("io_context exception : %s\n", e.what());
      }
    });
  }

  ~IoContextRunner() {
    guard_.reset();
    ctx_.stop();
    worker_.join();
  }
};

#endif
