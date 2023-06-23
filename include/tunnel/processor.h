#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"

namespace tunnel {

namespace detail {

static uint64_t GenerateId() {
  static std::atomic<uint64_t> id_{0};
  return id_.fetch_add(1);
}
} // namespace detail

template <typename T> class Processor {
public:
  Processor() : processor_id_(detail::GenerateId()) {}

  virtual async_simple::coro::Lazy<void> work() {}

  void PushInputPort(const Channel<T> &input_port) {
    auto it = std::find(input_ports.begin(), input_ports.end(), input_port);
    if (it != input_ports.end()) {
      throw std::runtime_error("PushInputPort error, duplicate id");
    }
    assert(input_port.GetOutputId() == GetId());
    input_ports.push_back(input_port);
  }

  void PushOutputPort(const Channel<T> &output_port) {
    auto it = std::find(output_ports.begin(), output_ports.end(), output_port);
    if (it != output_ports.end()) {
      throw std::runtime_error("PushOutputPort error, duplicate id");
    }
    assert(output_port.GetInputId() == GetId());
    output_ports.push_back(output_port);
  }

  virtual ~Processor() {}

  uint64_t GetId() noexcept { return processor_id_; }

  const std::vector<Channel<T>> &GetInputPorts() const { return input_ports; }

  std::vector<Channel<T>> &GetInputPorts() { return input_ports; }

  const std::vector<Channel<T>> &GetOutputPorts() const { return output_ports; }

  std::vector<Channel<T>> &GetOutputPorts() { return output_ports; }

private:
  uint64_t processor_id_;
  std::vector<Channel<T>> input_ports;
  std::vector<Channel<T>> output_ports;
};

template <typename T>
inline void connect(Processor<T> &input, Processor<T> &output) {
  Channel<T> channel(input.GetId(), output.GetId());
  input.PushOutputPort(channel);
  output.PushInputPort(channel);
}

} // namespace tunnel