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
#include "tunnel/processor.h"

using namespace tunnel;

TEST(ProcessorTest, basic) {
  Processor<int> node("test1");
  Processor<int> node2("test2");
  EXPECT_NE(node.GetId(), node2.GetId());
  EXPECT_EQ(node.GetName(), "test1");
  EXPECT_EQ(node2.GetName(), "test2");
  connect(node, node2, default_channel_size);
  EXPECT_THROW(node.work(), std::runtime_error);
  EXPECT_EQ(node2.GetInputPort(), true);
  EXPECT_EQ(node.GetOutputPort(), true);
}