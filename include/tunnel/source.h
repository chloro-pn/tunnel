#include "tunnel/channel.h"
#include "tunnel/processor.h"

#include <memory>
#include <optional>
#include <vector>

namespace tunnel {

template <typename T> class Source : public Processor<T> {
public:
  virtual async_simple::coro::Lazy<void> work() override {
    std::vector<Channel<T>> &outputs = this->GetOutputPorts();
    bool eof = false;
    while (true) {
      for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        std::optional<T> v = co_await generate();
        if (v.has_value()) {
          co_await (*it).GetQueue().Push(std::move(v));
        } else {
          eof = true;
          break;
        }
      }
      if (eof == true) {
        for (auto it = outputs.begin(); it != outputs.end(); ++it) {
          co_await (*it).GetQueue().Push(std::optional<T>{});
        }
        outputs.clear();
        co_return;
      }
    }
  }

  virtual async_simple::coro::Lazy<std::optional<T>> generate() = 0;
};

} // namespace tunnel