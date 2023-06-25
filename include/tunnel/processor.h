#ifndef TUNNEL_PROCESSOR_H
#define TUNNEL_PROCESSOR_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <optional>
#include <stdexcept>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"

namespace tunnel {

namespace detail {

static uint64_t GenerateId() {
  static std::atomic<uint64_t> id_{1};
  return id_.fetch_add(1);
}
}  // namespace detail

template <typename T>
class Processor {
 public:
  Processor() : processor_id_(detail::GenerateId()), input_count_(0), output_count_(0) {}

  virtual async_simple::coro::Lazy<void> work() { throw std::runtime_error("work function is not implemented"); }

  // input可能是一写多读或者多写一读的，
  // 如果是多写一读，则需要等待所有写的eof；
  // 如果是一写多读，则需要保证其他读也读到eof；
  async_simple::coro::Lazy<std::optional<T>> Pop() {
    while (true) {
      std::optional<T> value = co_await input_port.GetQueue().Pop();
      if (value.has_value()) {
        co_return value;
      }
      assert(input_count_ > 0);
      --input_count_;
      if (input_count_ == 0) {
        co_await input_port.GetQueue().Push(std::optional<T>{});
        co_return value;
      }
    }
  }

  void SetInputPort(const Channel<T> &input) {
    input_port = input;
    ++input_count_;
  }

  void SetOutputPort(const Channel<T> &output) {
    output_port = output;
    ++output_count_;
  }

  virtual ~Processor() {}

  uint64_t &GetId() noexcept { return processor_id_; }

  const Channel<T> &GetInputPort() const { return input_port; }

  Channel<T> &GetInputPort() { return input_port; }

  const Channel<T> &GetOutputPort() const { return output_port; }

  Channel<T> &GetOutputPort() { return output_port; }

 private:
  uint64_t processor_id_;
  Channel<T> input_port;
  Channel<T> output_port;

 protected:
  size_t input_count_;
  size_t output_count_;
};

template <typename T>
inline void connect(Processor<T> &input, Processor<T> &output) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size));
  input.SetOutputPort(channel);
  output.SetInputPort(channel);
}

template <typename T>
inline void connect(Processor<T> &input, Processor<T> &output, std::shared_ptr<BoundedQueue<std::optional<T>>> &queue) {
  Channel<T> channel(queue);
  input.SetOutputPort(channel);
  output.SetInputPort(channel);
}

}  // namespace tunnel

#endif
