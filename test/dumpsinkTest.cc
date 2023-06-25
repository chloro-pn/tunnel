#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/dump_sink.h"

using namespace tunnel;

struct DumpSinkTestClass {
  static size_t destruct_count_;

  DumpSinkTestClass() : value(std::make_unique<int>(0)) {}

  ~DumpSinkTestClass() {
    if (value) {
      destruct_count_ += 1;
    }
  }

  DumpSinkTestClass(DumpSinkTestClass&&) = default;
  DumpSinkTestClass& operator=(DumpSinkTestClass&&) = default;

  std::unique_ptr<int> value;
};

size_t DumpSinkTestClass::destruct_count_ = 0;

class DumpTestSource : public Source<DumpSinkTestClass> {
 public:
  virtual async_simple::coro::Lazy<std::optional<DumpSinkTestClass>> generate() override {
    if (count < 100) {
      count += 1;
      co_return DumpSinkTestClass{};
    }
    co_return std::optional<DumpSinkTestClass>{};
  }

 private:
  int count = 0;
};

TEST(dumpsinkTest, basic) {
  async_simple::executors::SimpleExecutor ex(1);
  DumpTestSource source;
  DumpSink<DumpSinkTestClass> sink;
  connect(source, sink);
  source.work().via(&ex).start([](async_simple::Try<void>) {});
  async_simple::coro::syncAwait(sink.work().via(&ex));
  EXPECT_EQ(DumpSinkTestClass::destruct_count_, 100);
}