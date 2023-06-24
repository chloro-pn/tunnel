#ifndef TUNNEL_TEST_DEFINE_H
#define TUNNEL_TEST_DEFINE_H

#include "tunnel/dispatch.h"
#include "tunnel/pipeline.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

#define ASYNC_SIMPLE_HAS_NOT_AIO
#include <functional>

#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"

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

#endif
