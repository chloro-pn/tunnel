#include "tunnel/pipeline.h"

#include "gtest/gtest.h"
#include "tunnel/dispatch.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

#define ASYNC_SIMPLE_HAS_NOT_AIO
#include <functional>
#include <iostream>

#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"

using namespace tunnel;

class SinkTest : public Sink<int> {
 public:
  virtual async_simple::coro::Lazy<void> consume(int &&value) override {
    if (callback) {
      uint64_t id = GetId();
      std::cout << id << " : sink " << this->input_count_ << std::endl;
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
      uint64_t id = GetId();
      std::cout << id << " : generate " << num << std::endl;
      co_return num;
    }
    co_return std::optional<int>{};
  }
  int num = 0;
};

class TransformTest : public SimpleTransform<int> {
 public:
  virtual async_simple::coro::Lazy<int> transform(int &&v) override {
    uint64_t id = GetId();
    std::cout << id << " : transform " << v << std::endl;
    co_return v * 2;
  }
};

class DispatchTest : public Dispatch<int> {
 public:
  DispatchTest(size_t size) : Dispatch<int>(size) {}

  virtual size_t dispatch(const int &v) {
    uint64_t id = GetId();
    std::cout << id << " : dispatch to " << v << std::endl;
    return v;
  }
};

int main() {
  async_simple::executors::SimpleExecutor ex(4);
  Pipeline<int> pipeline;
  uint64_t s1 = pipeline.AddSource(std::make_unique<SourceTest>());
  uint64_t s2 = pipeline.AddSource(std::make_unique<SourceTest>());

  uint64_t t1 = pipeline.AddTransform(s1, std::make_unique<TransformTest>());
  pipeline.AddTransform(s2, std::make_unique<TransformTest>());
  auto new_nodes = pipeline.DispatchTo(t1, std::make_unique<DispatchTest>(4));
  pipeline.Merge(std::make_unique<NoOpTransform<int>>(), new_nodes);
  auto sink = std::make_unique<SinkTest>();
  int result = 0;
  sink->callback = [&](int v) { result += v; };
  pipeline.SetSink(std::move(sink));
  async_simple::coro::syncAwait(std::move(pipeline).Run().via(&ex));
  if (result == 5050 * 4) {
    std::cout << "succ" << std::endl;
  } else {
    std::cout << "error " << std::endl;
  }
  return 0;
}