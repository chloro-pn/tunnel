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
#include "tunnel/util.h"

using namespace tunnel;

TEST(utilTest, basic) {
  int i = 0x1234;
  char b = *(char*)&i;
  if (Endian::Instance().GetEndianType() == Endian::Type::Big) {
    EXPECT_EQ(b, 0x12);
  } else {
    EXPECT_EQ(b, 0x34);
  }
  uint16_t a = flipByByte<uint16_t>(0x1234);
  EXPECT_EQ(a, 0x3412);
}
