#ifndef TUNNEL_SINK_H
#define TUNNEL_SINK_H

#include <memory>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Sink : public Processor<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T> &input = this->GetInputPort();
    while (true) {
      std::optional<T> value = co_await this->Pop();
      if (value.has_value()) {
        co_await consume(std::move(value).value());
      } else {
        co_return;
      }
    }
  }

  virtual async_simple::coro::Lazy<void> consume(T &&value) = 0;
};

}  // namespace tunnel

#endif
