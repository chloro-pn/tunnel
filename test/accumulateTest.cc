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
#include "tunnel/accumulate.h"
#include "tunnel/dump_sink.h"

using namespace tunnel;

class AccumulateTest : public Accumulate<int> {
 public:
  AccumulateTest(int& v) : sum_(v) {}

  virtual async_simple::coro::Lazy<void> consume(int&& v) override {
    sum_ += v;
    co_return;
  }

  virtual async_simple::coro::Lazy<int> generate() override { co_return sum_; }

  int& sum_;
};

TEST(accumulateTest, basic) {
  async_simple::executors::SimpleExecutor ex(2);
  Pipeline<int> pipeline;
  int result = 0;
  auto id = pipeline.AddSource(std::make_unique<SourceTest>());
  pipeline.AddTransform(id, std::make_unique<AccumulateTest>(result));
  pipeline.SetSink(std::make_unique<DumpSink<int>>());
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(result, 5050);
}
