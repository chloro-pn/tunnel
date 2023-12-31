#ifndef TUNNEL_CHANNEL_SOURCE_H
#define TUNNEL_CHANNEL_SOURCE_H

#include "tunnel/source.h"

namespace tunnel {

/*
 * ChannelSource read data from input channel (set by user) and write to output channel.
 */
template <typename T>
class ChannelSource : public Source<T> {
 public:
  ChannelSource(const std::string& name = "channel_source") : Source<T>(name) { this->input_count_ = 1; }

  // user have to set input channel before running pipeline, otherwise an exception will be thrown
  void SetInputChannel(const Channel<T>& input) {
    Channel<T>& input_channel = this->GetInputPort();
    input_channel = input;
  }

 private:
  virtual void before_work() override {
    Channel<T>& input_channel = this->GetInputPort();
    if (!input_channel) {
      throw std::runtime_error("channel source should set input channel before work");
    }
  }

  virtual async_simple::coro::Lazy<void> hosted_mode() override {
    Channel<T>& input = this->GetInputPort();
    Channel<T>& output = this->GetOutputPort();
    co_await this->close_input(input, this->input_count_);
    co_await this->close_output(output);
    co_return;
  }

  virtual async_simple::coro::Lazy<std::optional<T>> generate() override {
    Channel<T>& input_channel = this->GetInputPort();
    co_return co_await this->Pop(input_channel);
  }
};

}  // namespace tunnel

#endif
