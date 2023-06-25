#ifndef TUNNEL_DUMP_SINK_H
#define TUNNEL_DUMP_SINK_H

#include "async_simple/coro/Lazy.h"
#include "tunnel/channel.h"
#include "tunnel/sink.h"

namespace tunnel {

template <typename T>
class DumpSink : public Sink<T> {
 public:
  virtual async_simple::coro::Lazy<void> consume(T &&value) override {
    T v = std::move(value);
    co_return;
  }
};

}  // namespace tunnel

#endif
