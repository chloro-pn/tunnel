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

#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/channel_source.h"
#include "tunnel/pipeline.h"

using namespace tunnel;

TEST(channelSourceTest, basic) {
  Channel<int> channel;
  Pipeline<int> pipeline(PipelineOption{.bind_abort_channel = true});
  auto channel_source = std::make_unique<ChannelSource<int>>();
  Channel<int> input_channel(2);
  channel_source->SetInputChannel(input_channel);
  pipeline.AddSource(std::move(channel_source));
  auto sink = std::make_unique<SinkTest>();
  int count = 0;
  int sum = 0;
  sink->callback = [&](int v) {
    count += 1;
    sum += v;
  };
  pipeline.SetSink(std::move(sink));
  async_simple::executors::SimpleExecutor ex(2);

  auto task = [&]() -> async_simple::coro::Lazy<void> {
    co_await input_channel.GetQueue().Push(1);
    co_await input_channel.GetQueue().Push(2);
    co_await input_channel.GetQueue().Push(3);
    co_await input_channel.GetQueue().Push(std::optional<int>{});
    co_return;
  };

  task().via(&ex).start([](auto&&) {});

  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(count, 3);
  EXPECT_EQ(sum, 6);
}

TEST(channelSourceTest, empty_channel) {
  Pipeline<int> pipeline(PipelineOption{.bind_abort_channel = true});
  auto channel_source = std::make_unique<ChannelSource<int>>();
  pipeline.AddSource(std::move(channel_source));
  auto sink = std::make_unique<SinkTest>();
  sink->callback = [&](int v) {};
  pipeline.SetSink(std::move(sink));
  async_simple::executors::SimpleExecutor ex(2);
  auto results = async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  for (auto& each : results) {
    if (each.hasError()) {
      std::string result;
      try {
        std::rethrow_exception(each.getException());
      } catch (const std::exception& e) {
        result = e.what();
      }
      EXPECT_TRUE(result == "channel source should set input channel before work" ||
                  result == "throw by abort channel");
    }
  }
}
