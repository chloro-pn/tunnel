#ifndef TUNNEL_CHANNEL_SINK_H
#define TUNNEL_CHANNEL_SINK_H

#include "tunnel/sink.h"

namespace tunnel {

/*
 * ChannelSink read data for input channel and write to output channel (set by user).
 */
template <typename T>
class ChannelSink : public Sink<T> {
 public:
  ChannelSink(const std::string& name = "channel_sink") : Sink<T>(name) {}

  // user have to set output channel before running pipeline, otherwise an exception will be thrown
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

  // We have to write EOF to output channel
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
