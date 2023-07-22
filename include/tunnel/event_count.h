#ifndef TUNNEL_EVENT_COUNT_H
#define TUNNEL_EVENT_COUNT_H

#include <cassert>
#include <condition_variable>
#include <mutex>

namespace tunnel {

class EventCount {
 public:
  EventCount(size_t ec) : event_count_(ec), succ_count_(0), fail_count_(0) { assert(ec > 0); }

  void Succ() {
    std::unique_lock<std::mutex> guard(mut_);
    succ_count_ += 1;
    cv_.notify_one();
  }

  void Fail() {
    std::unique_lock<std::mutex> guard(mut_);
    fail_count_ += 1;
    cv_.notify_one();
  }

  void Wait() {
    std::unique_lock<std::mutex> guard(mut_);
    cv_.wait(guard, [&]() { return this->event_count_ == this->succ_count_ + this->fail_count_; });
  }

  bool allSucc() const { return succ_count_ == event_count_; }

 private:
  size_t event_count_;
  size_t succ_count_;
  size_t fail_count_;
  std::mutex mut_;
  std::condition_variable cv_;
};

}  // namespace tunnel

#endif
