#ifndef TUNNEL_CONCAT_H
#define TUNNEL_CONCAT_H

#include <cassert>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Concat : public Processor<T> {
 public:
  Concat() {}

  /*
   * 由pipeline接口保证，concat的input不存在一写多读和多写一读的情况，因此不需要记录
   * input_count_。
   */
  void AddInputPort(const Channel<T>& channel) { inputs_.push_back(channel); }

  size_t Size() const { return inputs_.size(); }

  virtual async_simple::coro::Lazy<void> work() override {
    assert(!inputs_.empty());
    Channel<T>& output = this->GetOutputPort();
    for (auto it = inputs_.begin(); it != inputs_.end(); ++it) {
      Channel<T>& input = *it;
      while (true) {
        std::optional<T> v = co_await input.GetQueue().Pop();
        if (v.has_value()) {
          co_await output.GetQueue().Push(std::move(v));
        } else {
          break;
        }
      }
    }
    co_await output.GetQueue().Push(std::optional<T>{});
  }

 private:
  std::vector<Channel<T>> inputs_;
};

template <typename T>
inline void connect(Processor<T>& input, Concat<T>& output) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size));
  input.SetOutputPort(channel);
  output.AddInputPort(channel);
}

}  // namespace tunnel

#endif
