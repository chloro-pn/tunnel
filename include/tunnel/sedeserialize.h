#ifndef TUNNEL_SEDESERIALIZE_H
#define TUNNEL_SEDESERIALIZE_H

#include <cstring>

#include "tunnel/tunnel_traits.h"
#include "tunnel/util.h"

namespace tunnel {

template <>
inline std::string Serialize(const uint32_t& v) {
  std::string buf;
  buf.resize(sizeof(uint32_t));
  uint32_t rv = v;
  if (Endian::Instance().GetEndianType() == Endian::Type::Little) {
    rv = flipByByte(v);
  }
  memcpy(&buf[0], &rv, sizeof(uint32_t));
  return buf;
}

template <>
inline uint32_t Deserialize(std::string_view view) {
  uint32_t v = {0};
  memcpy(&v, &view[0], view.size());
  if (Endian::Instance().GetEndianType() == Endian::Type::Little) {
    v = flipByByte(v);
  }
  return v;
}

}  // namespace tunnel

#endif
