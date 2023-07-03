#ifndef TUNNEL_CHANNEL_SINK_H
#define TUNNEL_CHANNEL_SINK_H

#include "tunnel/channel.h"
#include "tunnel/sink.h"

namespace tunnel {

template <typename T>
class ChannelSink : public Sink<T> {
 public:
  ChannelSink(const std::string& name = "channel_sink") : Sink<T>(name) {}

  void SetOutputChannel(const Channel<T>& channel) { output_channel = channel; }

  virtual void before_work() {
    if (!output_channel) {
      throw std::runtime_error("channel sink should set output channel before work");
    }
  }

  // We have to notify downstream EOF information
  virtual async_simple::coro::Lazy<void> after_work() override {
    co_await output_channel.GetQueue().Push(std::optional<T>{});
  }

  virtual async_simple::coro::Lazy<void> consume(T&& value) override {
    co_await output_channel.GetQueue().Push(std::move(value));
  }

 private:
  Channel<T> output_channel;
};

}  // namespace tunnel

#endif
