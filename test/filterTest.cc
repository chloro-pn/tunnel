#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/filter.h"

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
  connect(source, filter);
  connect(filter, sink);
  async_simple::executors::SimpleExecutor ex(2);
  source.work().via(&ex).start([](async_simple::Try<void>) {});
  filter.work().via(&ex).start([](async_simple::Try<void>) {});
  async_simple::coro::syncAwait(sink.work().via(&ex));
  EXPECT_EQ(result, 2500);
}