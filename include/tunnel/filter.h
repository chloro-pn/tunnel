#ifndef TUNNEL_FILTER_H
#define TUNNEL_FILTER_H

#include <optional>

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"
#include "tunnel/transform.h"

namespace tunnel {

template <typename T>
class Filter : public Transform<T> {
 public:
  virtual async_simple::coro::Lazy<void> work() {
    Channel<T> &output = this->GetOutputPort();
    while (true) {
      std::optional<T> v = co_await this->Pop();
      if (v.has_value()) {
        bool need_filter = filter(v.value());
        if (!need_filter) {
          output.GetQueue().Push(std::move(v));
        }
      } else {
        co_await output.GetQueue().Push(std::move(v));
        co_return;
      }
    }
  }

  virtual bool filter(const T &v) = 0;
};

}  // namespace tunnel

#endif