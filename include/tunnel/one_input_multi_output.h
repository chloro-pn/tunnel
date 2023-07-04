#ifndef TUNNEL_ONE_INPUT_MULTI_OUTPUT_H
#define TUNNEL_ONE_INPUT_MULTI_OUTPUT_H

#include <cassert>
#include <string>
#include <vector>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

/*
 * OneIMultiO have multiple output channels and one input channel.
 */
template <typename T>
class OneIMultiO : public Processor<T> {
 public:
  OneIMultiO(const std::string& name, size_t output_size) : Processor<T>(name), output_size_(output_size) {
    assert(output_size > 0);
  }

  void AddOutputPort(const Channel<T>& channel) { outputs_.emplace_back(channel); }

  virtual void before_work() {
    if (outputs_.size() != output_size_) {
      // todo : use fmt.
      throw std::runtime_error("one_input_multi_output check error");
    }
  }

  size_t Size() const { return output_size_; }

 protected:
  Channel<T>& GetChannel(size_t index) { return outputs_.at(index); }

 private:
  std::vector<Channel<T>> outputs_;
  size_t output_size_;
};

template <typename T>
inline void connect(OneIMultiO<T>& input, Processor<T>& output, size_t channel_size) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(channel_size));
  input.AddOutputPort(channel);
  output.SetInputPort(channel);
}

}  // namespace tunnel

#endif
