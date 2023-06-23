#pragma once

#include "queue/boundedqueue.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace tunnel {

template <typename T> class Channel {
public:
  constexpr static size_t default_channel_size = 3;
  using element_type = std::optional<T>;

  friend bool operator==(const Channel &lh, const Channel &rh) {
    return lh.input_id_ == rh.input_id_ && lh.output_id_ == rh.output_id_;
  }

  Channel(uint64_t in, uint64_t out)
      : input_id_(in), output_id_(out),
        queue_(std::make_shared<BoundedQueue<element_type>>(
            default_channel_size)) {}

  Channel(const Channel &) = default;
  Channel &operator=(const Channel &) = default;

  uint64_t GetInputId() const noexcept { return input_id_; }
  uint64_t GetOutputId() const noexcept { return output_id_; }

  BoundedQueue<element_type> &GetQueue() { return *queue_; }

private:
  uint64_t input_id_;
  uint64_t output_id_;
  std::shared_ptr<BoundedQueue<element_type>> queue_;
};

} // namespace tunnel