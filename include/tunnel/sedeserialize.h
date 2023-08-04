#ifndef TUNNEL_SEDESERIALIZE_H
#define TUNNEL_SEDESERIALIZE_H

#include <cstring>
#include <limits>
#include <string>

#include "tunnel/tunnel_traits.h"
#include "tunnel/util.h"

namespace tunnel {

#define DECLARE_INTEGRAL_SERIALIZE(integral_type)                           \
                                                                            \
  template <>                                                               \
  inline void Serialize(const integral_type& v, std::string& appender) {    \
    size_t old_size = appender.size();                                      \
    appender.resize(old_size + sizeof(integral_type));                      \
    integral_type rv = v;                                                   \
    if (Endian::Instance().GetEndianType() == Endian::Type::Little) {       \
      rv = flipByByte(v);                                                   \
    }                                                                       \
    memcpy(&appender[old_size], &rv, sizeof(integral_type));                \
    return;                                                                 \
  }                                                                         \
                                                                            \
  template <>                                                               \
  inline integral_type Deserialize(std::string_view view, size_t& offset) { \
    integral_type v = {0};                                                  \
    memcpy(&v, &view[offset], sizeof(integral_type));                       \
    offset += sizeof(integral_type);                                        \
    if (Endian::Instance().GetEndianType() == Endian::Type::Little) {       \
      v = flipByByte(v);                                                    \
    }                                                                       \
    return v;                                                               \
  }

DECLARE_INTEGRAL_SERIALIZE(uint32_t)
DECLARE_INTEGRAL_SERIALIZE(int)

template <>
inline void Serialize(const std::string& v, std::string& appender) {
  assert(v.size() <= std::numeric_limits<uint32_t>::max());
  Serialize(static_cast<uint32_t>(v.size()), appender);
  appender.append(v);
}

template <>
inline std::string Deserialize(std::string_view view, size_t& offset) {
  uint32_t v = Deserialize<uint32_t>(view, offset);
  const char* ptr = &view[offset];
  size_t len = v;
  offset += len;
  return std::string(ptr, len);
}

}  // namespace tunnel

#endif
