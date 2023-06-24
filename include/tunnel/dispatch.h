#ifndef TUNNEL_DISPATCH_H
#define TUNNEL_DISPATCH_H

#include <stdexcept>
#include <vector>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Dispatch : public Processor<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() {
    if (outputs_.size() != size_) {
      throw std::runtime_error("dispatch size mismatch");
    }
    while (true) {
      std::optional<T> v = co_await this->Pop();
      if (!v.has_value()) {
        for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
          co_await (*it).GetQueue().Push(std::optional<T>{});
        }
        co_return;
      } else {
        size_t output_index = dispatch(v.value()) % outputs_.size();
        co_await outputs_[output_index].GetQueue().Push(std::move(v));
      }
    }
  }

  explicit Dispatch(size_t size) : size_(size) {}

  void AddOutput(const Channel<T>& channel) {
    if (outputs_.size() < size_) {
      outputs_.push_back(channel);
    }
  }

  virtual size_t dispatch(const T& value) = 0;

  size_t GetSize() const { return size_; }

 private:
  const size_t size_;
  std::vector<Channel<T>> outputs_;
};

template <typename T>
inline void connect(Dispatch<T>& input, Processor<T>& output) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size));
  input.AddOutput(channel);
  output.SetInputPort(channel);
}

}  // namespace tunnel

#endif
