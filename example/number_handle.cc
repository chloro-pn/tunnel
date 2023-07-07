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
  explicit NumSource(const std::string& name = "") : Source<int>(name) {}

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
  explicit NumSink(int& s, const std::string& name = "") : Sink<int>(name), sum(s) {}

  virtual Lazy<void> consume(int&& v) override {
    sum += v;
    co_return;
  }

 private:
  int& sum;
};

class NumTransform : public SimpleTransform<int> {
 public:
  explicit NumTransform(const std::string& name = "") : SimpleTransform<int>(name) {}

  virtual async_simple::coro::Lazy<int> transform(int&& value) override { co_return value * 2; }
};

class NumFilter : public Filter<int> {
 public:
  explicit NumFilter(const std::string& name = "") : Filter<int>(name) {}

  virtual bool filter(const int& v) override { return v % 2 == 0; }
};

class NumDispatch : public Dispatch<int> {
 public:
  NumDispatch(size_t size, const std::string& name = "") : Dispatch<int>(size, name) {}

  virtual size_t dispatch(const int& value) override { return static_cast<size_t>(value); }
};

class NumAccumulate : public Accumulate<int> {
 public:
  explicit NumAccumulate(const std::string& name = "") : Accumulate<int>(name) {}

  virtual Lazy<void> consume(int&& v) override {
    tmp_sum += v;
    co_return;
  }

  virtual Lazy<int> generate() override { co_return tmp_sum; }

 private:
  int tmp_sum = 0;
};

// You can find the structural diagram of the pipeline constructed by this example
// in the file number_handle.md in the same directory
int main() {
  PipelineOption option;
  option.name = "number_handle_pipeline";
  option.bind_abort_channel = true;
  Pipeline<int> pipeline(option);
  int result = 0;
  auto s1_id = pipeline.AddSource(std::make_unique<NumSource>("source1"));
  pipeline.AddTransform(s1_id, std::make_unique<NumTransform>("transform"));

  auto s2_id = pipeline.AddSource(std::make_unique<NumSource>("source2"));
  pipeline.AddTransform(s2_id, std::make_unique<NumFilter>("filter"));

  auto next_id = pipeline.Merge();
  pipeline.DispatchFrom(next_id, std::make_unique<NumDispatch>(4, "dispatch"));
  // Add NumAccumulate post-node to all leaf nodes
  pipeline.AddTransform([]() { return std::make_unique<NumAccumulate>("accumulate"); });
  // Set NumSink node for all leaf nodes
  pipeline.SetSink(std::make_unique<NumSink>(result, "sink"));

  std::cout << "pipeline dump : " << std::endl;
  std::cout << pipeline.Dump() << std::endl;

  SimpleExecutor ex(1);
  syncAwait(std::move(pipeline).Run().via(&ex));
  std::cout << "the result should be " << 5050 * 2 + 2500 << std::endl;
  std::cout << "result : " << result << std::endl;
  return 0;
}