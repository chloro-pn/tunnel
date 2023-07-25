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

#include "executor/tunnel_executor.h"
#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/event_count.h"
#include "tunnel/pipeline.h"
#include "tunnel/socket_sink.h"
#include "tunnel/socket_source.h"

using namespace tunnel;

TEST(socketProcessorTest, basic) {
  IoContextRunner io;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  TunnelExecutor te(2);
  EventCount ec(2);
  Pipeline<uint32_t> pipeline;
  pipeline.AddSource(std::make_unique<SocketSource<uint32_t>>(io.ctx_, "127.0.0.1", 12345));
  auto sink = std::make_unique<SinkTest<uint32_t>>();
  size_t result = 0;
  sink->callback = [&](uint32_t v) { result += v; };
  pipeline.SetSink(std::move(sink));
  std::move(pipeline).Run().via(&te).start([&](async_simple::Try<void>&& r) {
    if (r.hasError()) {
      ec.Fail();
    } else {
      ec.Succ();
    }
  });

  Pipeline<uint32_t> pipe2;
  pipe2.AddSource(std::make_unique<SourceTest<uint32_t>>());
  pipe2.SetSink(std::make_unique<SocketSink<uint32_t>>(io.ctx_, "127.0.0.1", 12345));
  std::move(pipe2).Run().via(&te).start([&](async_simple::Try<void>&& r) {
    if (r.hasError()) {
      ec.Fail();
    } else {
      ec.Succ();
    }
  });

  ec.Wait();
  EXPECT_EQ(ec.allSucc(), true);
  EXPECT_EQ(result, 5050);
}