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

#include <string>

#include "gtest/gtest.h"
#include "tunnel/sedeserialize.h"
#include "tunnel/tunnel_traits.h"

namespace tunnel {

template <>
size_t GetBytes(const std::string& v) {
  return v.size();
}
}  // namespace tunnel

class TraitsTest {
 public:
  size_t tunnel_get_bytes() const { return 100; }
};

TEST(traitTest, basic) {
  EXPECT_EQ(tunnel::HasTunnelBytesMethod<int>, false);
  EXPECT_EQ(tunnel::HasTunnelGetBytesSpecialization<std::string>, true);
  EXPECT_EQ(tunnel::HasTunnelBytesMethod<TraitsTest>, true);
  EXPECT_EQ(tunnel::RecordTransferredBytes<TraitsTest>, true);
  EXPECT_EQ(tunnel::RecordTransferredBytes<std::string>, true);
  EXPECT_EQ(tunnel::RecordTransferredBytes<double>, false);
  EXPECT_EQ(tunnel::GetTransferredBytes(TraitsTest{}), 100);
}
