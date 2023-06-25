#ifndef TUNNEL_CHANNEL_H
#define TUNNEL_CHANNEL_H

#include <cstdint>
#include <memory>
#include <optional>

#include "queue/boundedqueue.h"

namespace tunnel {

constexpr static size_t default_channel_size = 1;

template <typename T>
class Channel {
 public:
  using element_type = std::optional<T>;

  Channel() : queue_(nullptr) {}

  Channel(std::shared_ptr<BoundedQueue<element_type>> queue) : queue_(queue) {}

  Channel(const Channel &) = default;
  Channel &operator=(const Channel &) = default;

  BoundedQueue<element_type> &GetQueue() { return *queue_; }

  operator bool() const { return queue_.operator bool(); }

 private:
  std::shared_ptr<BoundedQueue<element_type>> queue_;
};

}  // namespace tunnel

#endif
