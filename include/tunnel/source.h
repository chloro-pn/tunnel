#ifndef TUNNEL_SOURCE_H
#define TUNNEL_SOURCE_H

#include <memory>
#include <optional>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Source : public Processor<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T> &output = this->GetOutputPort();
    while (true) {
      std::optional<T> v = co_await generate();
      bool read_eof = !v.has_value();
      co_await output.GetQueue().Push(std::move(v));
      if (read_eof) {
        co_return;
      }
    }
  }

  virtual async_simple::coro::Lazy<std::optional<T>> generate() = 0;
};

}  // namespace tunnel

#endif
