#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/filter.h"

using namespace tunnel;
using namespace async_simple::coro;
using namespace async_simple::executors;

TEST(simpletransformTest, NoOpTest) {
  SourceTest source;
  NoOpTransform<int> noop;
  SinkTest sink;
  int result = 0;
  sink.callback = [&](int v) { result += v; };
  connect(source, noop);
  connect(noop, sink);

  SimpleExecutor ex(2);
  source.work().via(&ex).start([](Try<void>) {});
  noop.work().via(&ex).start([](Try<void>) {});
  syncAwait(sink.work().via(&ex));
  EXPECT_EQ(result, 5050);
}

class SimpleTransformTest : public SimpleTransform<int> {
 public:
  virtual async_simple::coro::Lazy<int> transform(int &&value) override { co_return value * 2; }
};

TEST(simpletransformTest, simpleTest) {
  SourceTest source;
  SimpleTransformTest transform;
  SinkTest sink;
  int result = 0;
  sink.callback = [&](int v) { result += v; };
  connect(source, transform);
  connect(transform, sink);

  SimpleExecutor ex(2);
  source.work().via(&ex).start([](Try<void>) {});
  transform.work().via(&ex).start([](Try<void>) {});
  syncAwait(sink.work().via(&ex));
  EXPECT_EQ(result, 5050 * 2);
}