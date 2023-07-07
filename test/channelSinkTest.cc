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
#include "tunnel/channel_sink.h"
#include "tunnel/pipeline.h"

using namespace tunnel;

TEST(channelSinkTest, basic) {
  Channel<int> channel;
  Pipeline<int> pipeline(PipelineOption{.bind_abort_channel = true});
  pipeline.AddSource(std::make_unique<SourceTest>());
  auto channel_sink = std::make_unique<ChannelSink<int>>();
  Channel<int> out_channel(2);
  channel_sink->SetOutputChannel(out_channel);
  int count = 0;
  int sum = 0;
  pipeline.SetSink(std::move(channel_sink));
  async_simple::executors::SimpleExecutor ex(2);

  auto task = [&]() -> async_simple::coro::Lazy<void> {
    while (true) {
      std::optional<int> v = co_await out_channel.GetQueue().Pop();
      if (v.has_value()) {
        count += 1;
        sum += v.value();
      } else {
        co_return;
      }
    }
  };

  task().via(&ex).start([](auto&&) {});

  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(count, 100);
  EXPECT_EQ(sum, 5050);
}
