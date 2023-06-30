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

#include <functional>

#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/dispatch.h"
#include "tunnel/dump_sink.h"
#include "tunnel/fork.h"
#include "tunnel/pipeline.h"

TEST(TestPipeline, basic) {
  Pipeline<int> pipeline;
  EXPECT_EQ(pipeline.IsCompleted(), true);
  for(int i = 0; i < 3; ++i) {
    pipeline.AddSource(std::make_unique<SourceTest>());
    EXPECT_EQ(pipeline.IsCompleted(), false);
    EXPECT_EQ(pipeline.GetSources().size(), i + 1);
    EXPECT_EQ(pipeline.GetSinks().size(), 0);
  }
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
  EXPECT_EQ(pipeline.GetSinks().size(), 1);
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(5050 * 3, result);
}

class TransformTest : public SimpleTransform<int> {
public:
  virtual async_simple::coro::Lazy<int> transform(int &&value) override {
    co_return value * 2;
  }
};

class TransformTest2 : public SimpleTransform<int> {
public:
  virtual async_simple::coro::Lazy<int> transform(int &&value) override {
    co_return value % 2 == 0 ? 0 : value;
  }
};

TEST(TestPipeline, merge) {
  Pipeline<int> pipeline;
  uint64_t s1_id = pipeline.AddSource(std::make_unique<SourceTest>());
  uint64_t s2_id = pipeline.AddSource(std::make_unique<SourceTest>());
  uint64_t s3_id = pipeline.AddSource(std::make_unique<SourceTest>());
  uint64_t noop_id = pipeline.Merge({s1_id, s2_id});
  pipeline.SetSink(std::make_unique<DumpSink<int>>(), {s3_id});
  pipeline.AddTransform(noop_id, std::make_unique<TransformTest>());

  uint64_t s4_id = pipeline.AddSource(std::make_unique<SourceTest>());
  pipeline.Merge(std::make_unique<TransformTest2>(), {s4_id});
  EXPECT_EQ(pipeline.CurrentLeaves().size(), 2);
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(5050 * 4 + 2500, result);
}

class DispatchTest : public Dispatch<int> {
public:
  explicit DispatchTest(size_t size) : Dispatch<int>(size) {}

  virtual size_t dispatch(const int& value) override {
    return static_cast<size_t>(value / 20);
  }
};

class CallbackTransform : public SimpleTransform<int> {
public:
  virtual async_simple::coro::Lazy<int> transform(int &&value) override {
    if(call_back_) {
      call_back_(value);
    }
    co_return value;
  }

  std::function<void(int)> call_back_;
};

TEST(TestPipeline, dispatch) {
  Pipeline<int> pipeline;
  uint64_t s1_id = pipeline.AddSource(std::make_unique<SourceTest>());
  auto nodes = pipeline.DispatchFrom(s1_id, std::make_unique<DispatchTest>(5));
  EXPECT_EQ(nodes.size(), 5);
  std::vector<size_t> counts(5, 0);
  size_t index = 0;
  for(auto& each : nodes) {
    auto tran_tmp = std::make_unique<CallbackTransform>();
    tran_tmp->call_back_ = [&, index](int v) { counts[index] += 1; };
    index += 1;
    pipeline.AddTransform(each, std::move(tran_tmp));
  }
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(5050, result);
  for(auto& each : counts) {
    EXPECT_EQ(each, 20);
  }
}

TEST(TestPipeline, concat) {
  Pipeline<int> pipeline;
  uint64_t s1 = pipeline.AddSource(std::make_unique<SourceTest>(150));
  uint64_t s2 = pipeline.AddSource(std::make_unique<SourceTest>(0));
  uint64_t s3 = pipeline.AddSource(std::make_unique<SourceTest>(500));
  pipeline.ConcatFrom({s2, s1, s3});
  auto sink = std::make_unique<SinkTest>();
  std::vector<int> values;
  sink->callback = [&](int v) { values.push_back(v); };
  pipeline.SetSink(std::move(sink));
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  int old_v = -1;
  for (auto it = values.begin(); it != values.end(); ++it) {
    EXPECT_TRUE(old_v < *it);
    old_v = *it;
  }
}

TEST(TestPipeline, channelfork) {
  Pipeline<int> pipeline;
  auto id = pipeline.AddSource(std::make_unique<SourceTest>());
  pipeline.ForkFrom(id, 3);
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
  async_simple::executors::SimpleExecutor ex(2);
  auto node_results = async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(5050 * 3, result);
  for (auto& each : node_results) {
    EXPECT_EQ(each.hasError(), false);
  }
}

TEST(TestPipeline, pipelineMerge) {
  Pipeline<int> p1(PipelineOption{.name = "pipe1"});
  auto s1 = p1.AddSource(std::make_unique<SourceTest>(0, "s1"));
  auto s12 = p1.AddSource(std::make_unique<SourceTest>(0, "s12"));
  auto sink1 = p1.SetSink(std::make_unique<SinkTest>("sink1"));
  Pipeline<int> p2(PipelineOption{.name = "pipe2"});
  auto s2 = p2.AddSource(std::make_unique<SourceTest>(0, "s2"));
  auto sink2 = p2.SetSink(std::make_unique<SinkTest>("sink2"));
  Pipeline<int> merge_pipe = MergePipeline<int>(std::move(p1), std::move(p2));
  EXPECT_EQ(merge_pipe.GetName(), "pipe1-merge-pipe2");
  EXPECT_TRUE(merge_pipe.IsSource(s1));
  EXPECT_TRUE(merge_pipe.IsSource(s12));
  EXPECT_FALSE(merge_pipe.IsSource(s2));
  EXPECT_TRUE(merge_pipe.IsSink(sink2));
  EXPECT_FALSE(merge_pipe.IsSink(sink1));
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(merge_pipe).Run().via(&ex));
}

TEST(TestPipeline, throwException) {
  async_simple::executors::SimpleExecutor ex(2);
  PipelineOption option;
  option.bind_abort_channel = true;
  Pipeline<int> pipeline(option);
  auto s1 = pipeline.AddSource(std::make_unique<SourceTest>());
  pipeline.AddTransform(s1, std::make_unique<TransformTest>());
  pipeline.SetSink(std::make_unique<ThrowSinkTest>());
  auto results = async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  for (auto& each : results) {
    if (each.hasError()) {
      std::string result;
      try {
        std::rethrow_exception(each.getException());
      } catch (const std::exception& e) {
        result = e.what();
      }
      EXPECT_TRUE(result == "throw sink test" || result == "throw by abort channel");
    }
  }
}