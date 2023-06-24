#ifndef TUNNEL_SIMPLE_TRANSFORM_H
#define TUNNEL_SIMPLE_TRANSFORM_H

#include <cassert>
#include <memory>
#include <optional>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"
#include "tunnel/transform.h"

namespace tunnel {

template <typename T>
class SimpleTransform : public Transform<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() override {
    Channel<T> &input = this->GetInputPort();
    Channel<T> &output = this->GetOutputPort();
    while (true) {
      std::optional<T> v = co_await this->Pop();
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

template <typename T>
class NoOpTransform : public SimpleTransform<T> {
 public:
  virtual async_simple::coro::Lazy<T> transform(T &&value) override { co_return std::move(value); }
};

}  // namespace tunnel

#endif
