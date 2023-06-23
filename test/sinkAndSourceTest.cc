#include "tunnel/pipeline.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"
#include "tunnel/transform.h"
#include "gtest/gtest.h"

#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include <functional>

using namespace tunnel;

class SinkTest : public Sink<int> {
public:
  virtual async_simple::coro::Lazy<void> consume(int &&value) override {
    if (callback) {
      callback(value);
    }
    co_return;
  }

  std::function<void(int)> callback;
};

class SourceTest : public Source<int> {
public:
  virtual async_simple::coro::Lazy<std::optional<int>> generate() override {
    if (num < 100) {
      num = num + 1;
      co_return num;
    }
    co_return std::optional<int>{};
  }
  int num = 0;
};

class TransformTest : public Transform<int> {
public:
  virtual async_simple::coro::Lazy<int> transform(int &&v) override {
    co_return v * 2;
  }
};

TEST(TestSinkSource, basic) {
  SinkTest sink;
  SourceTest source;
  SourceTest source2;
  TransformTest transform;
  int result = 0;
  sink.callback = [&](int v) { result += v; };
  EXPECT_EQ(sink.GetInputPorts().size(), 0);
  connect(source, sink);
  connect(source2, transform);
  connect(transform, sink);
  async_simple::executors::SimpleExecutor ex(2);
  source.work().via(&ex).start([](Try<void>) {});
  source2.work().via(&ex).start([](Try<void>) {});
  transform.work().via(&ex).start([](Try<void>) {});
  syncAwait(sink.work().via(&ex));
  EXPECT_EQ(result, 5050 * 3);
}

TEST(TestPipeline, basic) {
  async_simple::executors::SimpleExecutor ex(4);
  Pipeline<int> pipeline;
  pipeline.AddSource(std::make_unique<SourceTest>());
  auto s2 = std::make_unique<SourceTest>();
  uint64_t id = s2->GetId();
  pipeline.AddSource(std::move(s2));
  pipeline.AddTransformTo(id, std::make_unique<TransformTest>());
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(result, 5050 * 3);
}