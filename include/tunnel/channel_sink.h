#ifndef TUNNEL_CHANNEL_SINK_H
#define TUNNEL_CHANNEL_SINK_H

#include "tunnel/sink.h"

namespace tunnel {

template <typename T>
class ChannelSink : public Sink<T> {
 public:
  ChannelSink(const std::string& name = "channel_sink") : Sink<T>(name) {}

  void SetOutputChannel(const Channel<T>& channel) {
    Channel<T>& output_channel = this->GetOutputPort();
    output_channel = channel;
  }

  virtual void before_work() {
    Channel<T>& output_channel = this->GetOutputPort();
    if (!output_channel) {
      throw std::runtime_error("channel sink should set output channel before work");
    }
  }

  // We have to notify downstream EOF information
  virtual async_simple::coro::Lazy<void> after_work() override {
    Channel<T>& output_channel = this->GetOutputPort();
    co_await output_channel.GetQueue().Push(std::optional<T>{});
  }

  virtual async_simple::coro::Lazy<void> consume(T&& value) override {
    Channel<T>& output_channel = this->GetOutputPort();
    co_await output_channel.GetQueue().Push(std::move(value));
  }
};

}  // namespace tunnel

#endif
