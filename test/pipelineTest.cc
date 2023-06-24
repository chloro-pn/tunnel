#include "gtest/gtest.h"
#include "test_define.h"

TEST(TestPipeline, basic) {
  async_simple::executors::SimpleExecutor ex(4);
  Pipeline<int> pipeline;
  uint64_t s1 = pipeline.AddSource(std::make_unique<SourceTest>());
  uint64_t s2 = pipeline.AddSource(std::make_unique<SourceTest>());

  uint64_t t1 = pipeline.AddTransform(s1, std::make_unique<TransformTest>());
  pipeline.AddTransform(s2, std::make_unique<TransformTest>());
  auto new_nodes = pipeline.DispatchTo(t1, std::make_unique<DispatchTest>(4));
  pipeline.Merge(new_nodes);
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  EXPECT_EQ(result, 5050 * 4);
}