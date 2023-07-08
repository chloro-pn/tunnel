#ifndef TUNNEL_MULTI_INPUT_ONE_OUTPUT_H
#define TUNNEL_MULTI_INPUT_ONE_OUTPUT_H

#include <cassert>
#include <string>
#include <vector>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

/*
 * MultiIOneO have multiple input channels and one output channel.
 */
template <typename T>
class MultiIOneO : public Processor<T> {
 public:
  MultiIOneO(const std::string& name, size_t input_size) : Processor<T>(name), input_size_(input_size) {
    assert(input_size > 0);
  }

  void AddInputPort(const Channel<T>& channel) { inputs_.emplace_back(channel); }

  virtual void before_work() {
    if (inputs_.size() != input_size_) {
      // todo : use fmt.
      throw std::runtime_error("multi_input_one_output check error");
    }
  }

  virtual async_simple::coro::Lazy<void> hosted_mode() override {
    for (size_t i = 0; i < Size(); ++i) {
      Channel<T>& input = GetChannel(i);
      size_t current_input_count = 1;
      co_await this->close_input(input, current_input_count);
    }
    Channel<T>& output = this->GetOutputPort();
    co_await this->close_output(output);
    co_return;
  }

  size_t Size() const { return input_size_; }

 protected:
  Channel<T>& GetChannel(size_t index) { return inputs_.at(index); }

 private:
  std::vector<Channel<T>> inputs_;
  size_t input_size_;
};

template <typename T>
void connect(Processor<T>& input, MultiIOneO<T>& output, size_t channel_size) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(channel_size));
  input.SetOutputPort(channel);
  output.AddInputPort(channel);
}

}  // namespace tunnel

#endif
