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
#define protected public

#include <condition_variable>
#include <mutex>

#include "executor/executor.h"
#include "executor/timer.h"
#include "gtest/gtest.h"

TEST(timerTest, basic) {
  tunnel::TunnelExecutor ex(4);
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::mutex mut;
  std::condition_variable cv;
  bool called = false;
  ex.schedule(
      [&]() {
        std::unique_lock<std::mutex> guard(mut);
        called = true;
        cv.notify_one();
      },
      std::chrono::milliseconds(20));
  std::unique_lock<std::mutex> guard(mut);
  cv.wait(guard, [&]() { return called == true; });
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  EXPECT_TRUE(end - start >= std::chrono::milliseconds(20));
}
