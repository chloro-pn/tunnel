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
#include "tunnel/filter.h"
#include "tunnel/pipeline.h"

using namespace tunnel;

class FilterTest : public Filter<int> {
 public:
  virtual bool filter(const int &v) override { return v % 2 == 0; }
};

TEST(filterTest, basic) {
  SourceTest source;
  FilterTest filter;
  SinkTest sink;
  int result = 0;
  sink.callback = [&](int v) { result += v; };
  connect(source, filter, default_channel_size);
  connect(filter, sink, default_channel_size);
  async_simple::executors::SimpleExecutor ex(2);
  source.work().via(&ex).start([](async_simple::Try<void>) {});
  filter.work().via(&ex).start([](async_simple::Try<void>) {});
  async_simple::coro::syncAwait(sink.work().via(&ex));
  EXPECT_EQ(result, 2500);
}