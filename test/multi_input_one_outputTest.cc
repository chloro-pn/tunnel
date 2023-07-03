#include "gtest/gtest.h"
#include "test_define.h"
#include "tunnel/multi_input_one_output.h"

using namespace tunnel;
using namespace async_simple::coro;

class MultiIOneOTest : public MultiIOneO<int> {
 public:
  MultiIOneOTest(size_t input_size) : MultiIOneO("test", input_size) {}

  virtual async_simple::coro::Lazy<void> work() override { co_return; }
};

TEST(testMultiIOneO, basic) {
  MultiIOneOTest multi(3);
  Channel<int> abort_channel(2);
  multi.BindAbortChannel(abort_channel);
  try {
    syncAwait(multi.work_with_exception());
  } catch (const std::runtime_error& e) {
    EXPECT_EQ(e.what(), std::string("multi_input_one_output check error"));
  }
}