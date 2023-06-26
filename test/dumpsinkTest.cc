/*
 * Copyright 2023, chloro-pn;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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