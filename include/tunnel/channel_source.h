#ifndef TUNNEL_CHANNEL_SOURCE_H
#define TUNNEL_CHANNEL_SOURCE_H

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class ChannelSource : public Source<T> {
 public:
  ChannelSource(const std::string& name = "channel_source") : Source<T>(name) {}

  void SetInputChannel(const Channel<T>& input) { input_channel = input; }

  virtual void before_work() override {
    if (!input_channel) {
      throw std::runtime_error("channel source should set input channel before work");
    }
  }

  virtual async_simple::coro::Lazy<std::optional<T>> generate() override {
    size_t input_count = 1;
    co_return co_await this->Pop(input_channel, input_count);
  }

 private:
  Channel<T> input_channel;
};

}  // namespace tunnel

#endif
