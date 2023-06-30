#ifndef TUNNEL_MULTI_INPUT_ONE_OUTPUT_H
#define TUNNEL_MULTI_INPUT_ONE_OUTPUT_H

#include <cassert>
#include <string>
#include <vector>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

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

  size_t Size() const { return inputs_.size(); }

 protected:
  Channel<T>& GetChannel(size_t index) { return inputs_.at(index); }

 private:
  std::vector<Channel<T>> inputs_;
  size_t input_size_;
};

template <typename T>
inline void connect(Processor<T>& input, MultiIOneO<T>& output, size_t channel_size) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(channel_size));
  input.SetOutputPort(channel);
  output.AddInputPort(channel);
}

}  // namespace tunnel

#endif
