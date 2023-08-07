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

 private:
  virtual void before_work() {
    Channel<T>& output_channel = this->GetOutputPort();
    if (!output_channel) {
      throw std::runtime_error("channel sink should set output channel before work");
    }
  }

  virtual async_simple::coro::Lazy<void> hosted_mode() override {
    Channel<T>& input = this->GetInputPort();
    Channel<T>& output = this->GetOutputPort();
    co_await this->close_input(input, this->input_count_);
    co_await this->close_output(output);
    co_return;
  }

  // We have to write EOF to output channel
  virtual async_simple::coro::Lazy<void> after_work() override {
    Channel<T>& output_channel = this->GetOutputPort();
    co_await output_channel.GetQueue().Push(std::optional<T>{});
    co_return;
  }

  virtual async_simple::coro::Lazy<void> consume(T&& value) override {
    Channel<T>& output_channel = this->GetOutputPort();
    co_await this->Push(std::move(value), output_channel);
    co_return;
  }
};

}  // namespace tunnel

#endif
