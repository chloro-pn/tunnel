#ifndef TUNNEL_TRAITS_H
#define TUNNEL_TRAITS_H

#include <concepts>
#include <cstddef>
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

}  // namespace tunnel

#endif
