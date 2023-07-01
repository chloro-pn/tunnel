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

#include <atomic>

#include "executor/executor.h"
#include "gtest/gtest.h"

using namespace tunnel;

TEST(executorText, basic) {
  TunnelExecutor ex(4);
  EXPECT_EQ(ex.GetThreadNum(), 4);
  EXPECT_EQ(ex.currentThreadInExecutor(), false);
  std::atomic<uint64_t> count{0};
  for (size_t i = 0; i < 10000; ++i) {
    ex.schedule([&]() { count.fetch_add(1); });
  }
  ex.Stop();
  EXPECT_EQ(count.load(), 10000);
}
