#ifndef TUNNEL_EVENT_COLLECTOR_H
#define TUNNEL_EVENT_COLLECTOR_H

#include <chrono>
#include <string>
#include <thread>

#include "async_simple/Executor.h"
#include "rigtorp/MPMCQueue.h"

namespace tunnel {

enum class EventType : uint8_t {
  BEFORE_READ_INPUT,
  AFTER_READ_INPUT,
  BEFORE_WRITE_OUTPUT,
  AFTER_WRITE_OUTPUT,
  WORK_START,
  WORK_END,
  HOSTED_MODE,
  PIPELINE_END,
};

struct EventInfo {
  std::chrono::system_clock::time_point time_point_;
  async_simple::Executor* ex_;
  uint64_t node_id_;
  std::string node_name_;
  size_t input_channel_index_ = 0;
  size_t output_channel_index_ = 0;
  EventType event_type_;

  EventInfo() = default;

  EventInfo(EventInfo&&) = default;
  EventInfo& operator=(EventInfo&&) = default;

  static EventInfo PipelineEnd() {
    EventInfo ei;
    ei.time_point_ = std::chrono::system_clock::now();
    ei.event_type_ = EventType::PIPELINE_END;
    return ei;
  }

  static EventInfo WorkStart(uint64_t id, const std::string& name, async_simple::Executor* ex) {
    EventInfo ei;
    ei.time_point_ = std::chrono::system_clock::now();
    ei.ex_ = ex;
    ei.node_id_ = id;
    ei.node_name_ = name;
    ei.event_type_ = EventType::WORK_START;
    return ei;
  }

  static EventInfo WorkEnd(uint64_t id, const std::string& name, async_simple::Executor* ex) {
    EventInfo ei;
    ei.time_point_ = std::chrono::system_clock::now();
    ei.ex_ = ex;
    ei.node_id_ = id;
    ei.node_name_ = name;
    ei.event_type_ = EventType::WORK_END;
    return ei;
  }

  static EventInfo HostedMode(uint64_t id, const std::string& name, async_simple::Executor* ex) {
    EventInfo ei;
    ei.time_point_ = std::chrono::system_clock::now();
    ei.ex_ = ex;
    ei.node_id_ = id;
    ei.node_name_ = name;
    ei.event_type_ = EventType::HOSTED_MODE;
    return ei;
  }

  static EventInfo BeforeReadInput(uint64_t id, const std::string& name, async_simple::Executor* ex,
                                   size_t input_index = 0) {
    EventInfo ei;
    ei.time_point_ = std::chrono::system_clock::now();
    ei.ex_ = ex;
    ei.node_id_ = id;
    ei.node_name_ = name;
    ei.input_channel_index_ = input_index;
    ei.event_type_ = EventType::BEFORE_READ_INPUT;
    return ei;
  }

  static EventInfo AfterReadInput(uint64_t id, const std::string& name, async_simple::Executor* ex,
                                  size_t input_index = 0) {
    EventInfo ei;
    ei.time_point_ = std::chrono::system_clock::now();
    ei.ex_ = ex;
    ei.node_id_ = id;
    ei.node_name_ = name;
    ei.input_channel_index_ = input_index;
    ei.event_type_ = EventType::AFTER_READ_INPUT;
    return ei;
  }
};

class EventCollector {
 public:
  EventCollector() : start_(false) {}

  EventCollector(EventCollector&&) = default;
  EventCollector& operator=(EventCollector&&) = default;

  void Start() {
    if (start_ == true) {
      return;
    }
    queue_.reset(new rigtorp::MPMCQueue<EventInfo>(102400));
    work_ = std::thread([&]() { this->backend_handle(); });
    start_ = true;
  }

  void Collect(EventInfo&& event_info) { queue_->push(std::move(event_info)); }

  void Handle(EventInfo&& ei) {}

  void Stop() {
    if (work_.joinable()) {
      work_.join();
    }
  }

  ~EventCollector() { Stop(); }

 private:
  void backend_handle() {
    while (true) {
      EventInfo ei;
      bool succ = queue_->try_pop(ei);
      if (succ == true) {
        bool eof = ei.event_type_ == EventType::PIPELINE_END;
        Handle(std::move(ei));
        if (eof == true) {
          break;
        }
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
    }
  }

  bool start_;
  std::unique_ptr<rigtorp::MPMCQueue<EventInfo>> queue_;
  std::thread work_;
};

}  // namespace tunnel

#endif
