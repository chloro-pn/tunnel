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
  connect(source, noop, default_channel_size);
  connect(noop, sink, default_channel_size);

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
  connect(source, transform, default_channel_size);
  connect(transform, sink, default_channel_size);

  SimpleExecutor ex(2);
  source.work().via(&ex).start([](Try<void>) {});
  transform.work().via(&ex).start([](Try<void>) {});
  syncAwait(sink.work().via(&ex));
  EXPECT_EQ(result, 5050 * 2);
}
