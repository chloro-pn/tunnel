#ifndef TUNNEL_PACKAGE_H
#define TUNNEL_PACKAGE_H

#include <stdexcept>
#include <string>
#include <string_view>

#include "tunnel/sedeserialize.h"
#include "tunnel/tunnel_traits.h"

namespace tunnel {

struct package {
  bool eof;
  std::string bin_data;
};

template <>
inline void Serialize(const package& v, std::string& appender) {
  appender.push_back(v.eof == true ? 'e' : 'n');
  Serialize(v.bin_data, appender);
  return;
}

template <>
inline package Deserialize(std::string_view view, size_t& offset) {
  package v;
  if (view.empty()) {
    throw std::runtime_error("deserialize package error, empty");
  }
  if (view[offset] != 'e' && view[offset] != 'n') {
    throw std::runtime_error("deserialize package error, parse invalid field");
  }
  v.eof = view[offset] == 'e' ? true : false;
  offset += sizeof(char);
  v.bin_data = Deserialize<std::string>(view, offset);
  return v;
}

}  // namespace tunnel

#endif
