#include "tunnel/pipeline.h"

#include <functional>
#include <iostream>

#include "async_simple/Try.h"
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "tunnel/dispatch.h"
#include "tunnel/filter.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

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

class TransformTest : public SimpleTransform<int> {
 public:
  virtual async_simple::coro::Lazy<int> transform(int &&v) override { co_return v * 2; }
};

class DispatchTest : public Dispatch<int> {
 public:
  DispatchTest(size_t size) : Dispatch<int>(size) {}

  virtual size_t dispatch(const int &v) { return v; }
};

class FilterTest : public Filter<int> {
 public:
  virtual bool filter(const int &v) override { return v % 2 == 0; }
};

// todo
int main() {
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
  std::cout << result << std::endl;
  return 0;
}