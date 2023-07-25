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
#include "tunnel/empty_source.h"
#include "tunnel/pipeline.h"

using namespace tunnel;

TEST(emptySourceTest, basic) {
  Pipeline<int> pipeline;
  pipeline.AddSource(std::make_unique<EmptySource<int>>());
  auto sink = std::make_unique<SinkTest<>>();
  int count = 0;
  sink->callback = [&](int v) { count += 1; };
  pipeline.SetSink(std::move(sink));
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(count, 0);
}
