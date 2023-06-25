#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/dump_sink.h"
#include "tunnel/dispatch.h"

#include <functional>

TEST(TestPipeline, basic) {
  Pipeline<int> pipeline;
  EXPECT_EQ(pipeline.IsCompleted(), true);
  for(int i = 0; i < 3; ++i) {
    pipeline.AddSource(std::make_unique<SourceTest>());
    EXPECT_EQ(pipeline.IsCompleted(), false);
  }
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
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
  auto nodes = pipeline.DispatchTo(s1_id, std::make_unique<DispatchTest>(5));
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