#ifndef TUNNEL_TRAITS_H
#define TUNNEL_TRAITS_H

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace tunnel {

/*
 * 用户可以通过在tunnel命名空间提供T类型的GetBytes函数模板的特化版本，或者给T类型添加一个size_t tunnel_get_bytes()
 * const成员函数。 如果这样做的话，统计信息中将包含每个节点读取与写入的字节数。 tunnel_get_bytes具有更高的优先级
 */

template <typename T>
size_t GetBytes(const T& v) = delete;

template <typename T>
concept HasTunnelBytesMethod = requires(const T& v) {
  { v.tunnel_get_bytes() } -> std::same_as<size_t>;
};

template <typename T>
concept HasTunnelGetBytesSpecialization = requires(const T& v) {
  { GetBytes(v) } -> std::same_as<size_t>;
};

template <typename T>
concept RecordTransferredBytes = HasTunnelGetBytesSpecialization<T> || HasTunnelBytesMethod<T>;

template <typename T>
requires RecordTransferredBytes<T> size_t GetTransferredBytes(const T& v) {
  if constexpr (HasTunnelBytesMethod<T>) {
    return v.tunnel_get_bytes();
  } else {
    return GetBytes(v);
  }
}

template <typename T>
std::string Serialize(const T& v) = delete;

template <>
inline std::string Serialize(const uint32_t& v) {
  std::string buf;
  buf.resize(sizeof(uint32_t));
  memcpy(&buf[0], &v, sizeof(uint32_t));
  return buf;
}

template <typename T>
concept HasTunnelSerializeSpecialization = requires(const T& v) {
  { Serialize(v) } -> std::same_as<std::string>;
};

template <typename T>
T Deserialize(std::string_view view) = delete;

template <>
inline uint32_t Deserialize(std::string_view view) {
  uint32_t v = {0};
  memcpy(&v, &view[0], view.size());
  return v;
}

template <typename T>
concept HasTunnelDeserializeSpecialization = requires(std::string_view v) {
  { Deserialize<T>(v) } -> std::same_as<T>;
};

}  // namespace tunnel

#endif
