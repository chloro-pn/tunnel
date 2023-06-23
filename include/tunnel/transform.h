#include "tunnel/channel.h"
#include "tunnel/processor.h"

#include <stdexcept>

namespace tunnel {

template <typename T> class Transform : public Processor<T> {
public:
  virtual async_simple::coro::Lazy<void> work() override {
    std::vector<Channel<T>> &inputs = this->GetInputPorts();
    std::vector<Channel<T>> &outputs = this->GetOutputPorts();
    if (inputs.size() != 1 || outputs.size() != 1) {
      throw std::runtime_error(
          "Transform can only has one input port and one output port");
    }
    Channel<T> &input = inputs[0];
    Channel<T> &output = outputs[0];
    while (true) {
      std::optional<T> v = co_await input.GetQueue().Pop();
      if (v.has_value()) {
        T new_v = co_await transform(std::move(v).value());
        co_await output.GetQueue().Push(std::move(new_v));
      } else {
        co_await output.GetQueue().Push(std::move(v));
        co_return;
      }
    }
  }

  virtual async_simple::coro::Lazy<T> transform(T &&value) = 0;
};

} // namespace tunnel