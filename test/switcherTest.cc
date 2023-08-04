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

#include <chrono>
#include <thread>

#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "awaiter/asio/socket.h"
#include "executor/tunnel_executor.h"
#include "gtest/gtest.h"
#include "switcher/switcher.h"
#include "switcher/switcher_client.h"
#include "test_define.h"
#include "tunnel/pipeline.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

using namespace tunnel;
using namespace async_simple::coro;

class SwitcherSource : public Source<int> {
 public:
  SwitcherSource(asio::io_context& ctx) : client_(ctx, "127.0.0.1", 12345), connect_(false), count_(0) {}

  virtual async_simple::coro::Lazy<std::optional<int>> generate() override {
    if (count_ == 10) {
      co_return std::optional<int>{};
    }
    if (connect_ == false) {
      co_await client_.Connect();
    }
    int v = co_await client_.Pop<int>("test");
    count_ += 1;
    co_return v;
  }

 private:
  SwitcherClient client_;
  bool connect_;
  size_t count_;
};

class SwitcherSink : public Sink<int> {
 public:
  SwitcherSink(asio::io_context& ctx) : client_(ctx, "127.0.0.1", 12345), connect_(false) {}

  virtual async_simple::coro::Lazy<void> consume(int&& value) override {
    if (connect_ == false) {
      co_await client_.Connect();
    }
    co_await client_.Push("test", value);
    co_return;
  }

 private:
  SwitcherClient client_;
  bool connect_;
};

TEST(switcherTest, serialize) {
  request_package rp;
  rp.topic = "test_for_switcher";
  rp.type = request_type_push;
  std::string appender;
  Serialize(rp, appender);
  auto nrp = Deserialize<request_package>(appender);
  EXPECT_EQ(rp.topic, nrp.topic);
  EXPECT_EQ(rp.type, nrp.type);
}

TEST(switcherTest, basic) {
  IoContextRunner io;
  TunnelExecutor ex(2);
  Switcher server(asio::ip::tcp::endpoint{asio::ip::address::from_string("127.0.0.1"), 12345});
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  server.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Pipeline<int> pipe;
  pipe.AddSource(std::make_unique<SourceTest<int, 10>>());
  pipe.SetSink(std::make_unique<SwitcherSink>(io.ctx_));
  pipe.AddSource(std::make_unique<SwitcherSource>(io.ctx_));
  auto sink = std::make_unique<SinkTest<int>>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipe.SetSink(std::move(sink));
  async_simple::coro::syncAwait(std::move(pipe).Run().via(&ex));
  EXPECT_EQ(result, 55);
  server.Stop();
}
