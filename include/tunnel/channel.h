/*
 * Copyright 2023, chloro-pn;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TUNNEL_CHANNEL_H
#define TUNNEL_CHANNEL_H

#include <cstdint>
#include <memory>
#include <optional>

#include "queue/boundedqueue.h"

namespace tunnel {

constexpr static size_t default_channel_size = 1;

// Channel is the encapsulation of BoundedQueue, and multiple channels can share the same BoundedQueue.
// A Processor can hold multiple channels, which are classified as input and output types.
template <typename T>
class Channel {
 public:
  using element_type = std::optional<T>;

  Channel() : queue_(nullptr), index_(0) {}

  Channel(std::shared_ptr<BoundedQueue<element_type>> queue) : queue_(queue), index_(0) {}

  Channel(size_t capacity) : queue_(std::make_shared<BoundedQueue<element_type>>(capacity)) {}

  Channel(const Channel &) = default;
  Channel &operator=(const Channel &) = default;

  BoundedQueue<element_type> &GetQueue() { return *queue_; }

  operator bool() const { return queue_.operator bool(); }

  void SetIndex(size_t index) noexcept { index_ = index; }

  size_t GetIndex() const noexcept { return index_; }

  void reset() { queue_.reset(); }

 private:
  std::shared_ptr<BoundedQueue<element_type>> queue_;
  size_t index_;
};

}  // namespace tunnel

#endif
