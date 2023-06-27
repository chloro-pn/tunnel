#include <iostream>

#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "tunnel/accumulate.h"
#include "tunnel/dispatch.h"
#include "tunnel/filter.h"
#include "tunnel/pipeline.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

using namespace tunnel;
using namespace async_simple::coro;
using namespace async_simple::executors;

class NumSource : public Source<int> {
 public:
  virtual Lazy<std::optional<int>> generate() override {
    if (count < 100) {
      count += 1;
      co_return count;
    }
    co_return std::optional<int>{};
  }

 private:
  int count = 0;
};

class NumSink : public Sink<int> {
 public:
  explicit NumSink(int& s) : sum(s) {}

  virtual Lazy<void> consume(int&& v) override {
    sum += v;
    co_return;
  }

 private:
  int& sum;
};

class NumTransform : public SimpleTransform<int> {
 public:
  virtual async_simple::coro::Lazy<int> transform(int&& value) override { co_return value * 2; }
};

class NumFilter : public Filter<int> {
 public:
  virtual bool filter(const int& v) override { return v % 2 == 0; }
};

class NumDispatch : public Dispatch<int> {
 public:
  NumDispatch(size_t size) : Dispatch<int>(size) {}

  virtual size_t dispatch(const int& value) override { return static_cast<size_t>(value); }
};

class NumAccumulate : public Accumulate<int> {
 public:
  virtual Lazy<void> consume(int&& v) override {
    tmp_sum += v;
    co_return;
  }

  virtual Lazy<int> generate() override { co_return tmp_sum; }

 private:
  int tmp_sum = 0;
};

// You can find the structural diagram of the pipeline constructed by this instance 
// in the file number_handle.md in this directory
int main() {
  Pipeline<int> pipeline;
  int result = 0;
  auto s1_id = pipeline.AddSource(std::make_unique<NumSource>());
  pipeline.AddTransform(s1_id, std::make_unique<NumTransform>());

  auto s2_id = pipeline.AddSource(std::make_unique<NumSource>());
  pipeline.AddTransform(s2_id, std::make_unique<NumFilter>());

  auto next_id = pipeline.Merge();
  pipeline.DispatchFrom(next_id, std::make_unique<NumDispatch>(4));
  // Add NumAccumulate post-node to all leaf nodes
  pipeline.AddTransform([]() { return std::make_unique<NumAccumulate>(); });
  // Set NumSink node for all leaf nodes
  pipeline.SetSink(std::make_unique<NumSink>(result));

  SimpleExecutor ex(4);
  syncAwait(std::move(pipeline).Run().via(&ex));
  std::cout << "the result should be " << 5050 * 2 + 2500 << std::endl;
  std::cout << "result : " << result << std::endl;
  return 0;
}