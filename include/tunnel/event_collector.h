#ifndef TUNNEL_EVENT_COLLECTOR_H
#define TUNNEL_EVENT_COLLECTOR_H

#include <chrono>
#include <map>
#include <optional>
#include <string>

namespace tunnel {

struct EventCollector {
  EventCollector(uint64_t id = 0, const std::string& name = "") : processor_id_(id), processor_name_(name) {}

  void WorkStart() { work_start_ = std::chrono::system_clock::now(); }

  void WorkEnd() { work_end_ = std::chrono::system_clock::now(); }

  void EnterHostedMode() { enter_hosted_mode_ = std::chrono::system_clock::now(); }

  void InputPortRead(size_t index, bool eof = false) {
    if (input_ports_statistic_.count(index) == 0) {
      port_statistic new_port;
      new_port.first_time_ = std::chrono::system_clock::now();
      input_ports_statistic_.insert({index, new_port});
    }
    auto it = input_ports_statistic_.find(index);
    it->second.count_ += 1;
    if (eof == true) {
      it->second.last_time_ = std::chrono::system_clock::now();
    }
  }

  void OutputPortWrite(size_t index, bool eof = false) {
    if (output_ports_statistic_.count(index) == 0) {
      port_statistic new_port;
      new_port.first_time_ = std::chrono::system_clock::now();
      output_ports_statistic_.insert({index, new_port});
    }
    auto it = output_ports_statistic_.find(index);
    it->second.count_ += 1;
    if (eof == true) {
      it->second.last_time_ = std::chrono::system_clock::now();
    }
  }

  uint64_t processor_id_;
  std::string processor_name_;
  std::chrono::system_clock::time_point work_start_;
  std::chrono::system_clock::time_point work_end_;
  std::optional<std::chrono::system_clock::time_point> enter_hosted_mode_;

  struct port_statistic {
    // read / write total number
    size_t count_ = 0;
    // the time_point of first read / write
    std::chrono::system_clock::time_point first_time_;
    // the time_point of last read / write
    std::chrono::system_clock::time_point last_time_;
  };

  std::map<size_t, port_statistic> input_ports_statistic_;
  std::map<size_t, port_statistic> output_ports_statistic_;
};

}  // namespace tunnel

#endif
