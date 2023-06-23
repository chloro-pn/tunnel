#pragma once

#include "tunnel/channel.h"
#include "tunnel/processor.h"

#include <memory>
#include <vector>

namespace tunnel {

template <typename T> class Sink : public Processor<T> {
public:
  virtual async_simple::coro::Lazy<void> work() override {
    std::vector<Channel<T>> &inputs = this->GetInputPorts();
    while (true) {
      if (inputs.empty()) {
        co_return;
      }
      for (auto it = inputs.begin(); it != inputs.end();) {
        std::optional<T> value = co_await (*it).GetQueue().Pop();
        // queue eof
        if (!value.has_value()) {
          it = inputs.erase(it);
          continue;
        }
        co_await consume(std::move(value).value());
        it = it + 1;
      }
    }
    co_return;
  }

  virtual async_simple::coro::Lazy<void> consume(T &&value) = 0;
};

} // namespace tunnel