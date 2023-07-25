#ifndef TUNNEL_PACKAGE_H
#define TUNNEL_PACKAGE_H

#include <stdexcept>
#include <string>
#include <string_view>

#include "tunnel/tunnel_traits.h"

namespace tunnel {

struct package {
  bool eof;
  std::string bin_data;
};

template <>
inline std::string Serialize(const package& v) {
  std::string buf;
  buf.push_back(v.eof == true ? 'e' : 'n');
  buf.append(v.bin_data);
  return buf;
}

template <>
inline package Deserialize(std::string_view view) {
  package v;
  if (view.empty()) {
    throw std::runtime_error("deserialize package error, empty");
  }
  if (view[0] != 'e' && view[0] != 'n') {
    throw std::runtime_error("deserialize package error, parse invalid field");
  }
  v.eof = view[0] == 'e' ? true : false;
  v.bin_data = view.substr(1);
  return v;
}

}  // namespace tunnel

#endif
