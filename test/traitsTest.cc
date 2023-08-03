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

struct TestSerialize {
  int a;
  uint32_t b;
  std::string c;
};

namespace tunnel {

template <>
inline void Serialize(const TestSerialize& v, std::string& appender) {
  Serialize<int>(v.a, appender);
  Serialize<uint32_t>(v.b, appender);
  Serialize<std::string>(v.c, appender);
}

template <>
inline TestSerialize Deserialize(std::string_view view, size_t& offset) {
  TestSerialize ts;
  ts.a = Deserialize<int>(view, offset);
  ts.b = Deserialize<uint32_t>(view, offset);
  ts.c = Deserialize<std::string>(view, offset);
  return ts;
}

}  // namespace tunnel

TEST(traitTest, serialize) {
  EXPECT_EQ(tunnel::HasTunnelSerializeSpecialization<TestSerialize>, true);
  EXPECT_EQ(tunnel::HasTunnelDeserializeSpecialization<TestSerialize>, true);
  TestSerialize ts;
  ts.a = 0;
  ts.b = 100;
  ts.c = "hello world";
  std::string buf;
  tunnel::Serialize<TestSerialize>(ts, buf);
  auto ts2 = tunnel::Deserialize<TestSerialize>(buf);
  EXPECT_EQ(ts2.a, ts.a);
  EXPECT_EQ(ts2.b, ts.b);
  EXPECT_EQ(ts2.c, ts.c);
}
