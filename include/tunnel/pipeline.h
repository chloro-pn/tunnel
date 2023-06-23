#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "tunnel/sink.h"
#include "tunnel/source.h"
#include "tunnel/transform.h"

#include "async_simple/Executor.h"
#include "async_simple/coro/Collect.h"

namespace tunnel {

template <typename T> class Pipeline {
public:
  Pipeline() : completed_(false) {}

  void AddSource(std::unique_ptr<Source<T>> &&source) {
    uint64_t id = source->GetId();
    node_check(id);
    nodes_.insert({id, std::move(source)});
    leaves_.insert(id);
  }

  void AddTransformTo(uint64_t leaf_id,
                      std::unique_ptr<Transform<T>> &&transform) {
    uint64_t trans_id = transform->GetId();
    node_check(trans_id);
    if (leaves_.find(leaf_id) == leaves_.end()) {
      throw std::runtime_error("invalid leaf node for pipeline");
    }
    assert(nodes_.find(leaf_id) != nodes_.end());
    connect(*nodes_[leaf_id], *transform);
    leaves_.erase(leaf_id);
    leaves_.insert(trans_id);
    nodes_.insert({trans_id, std::move(transform)});
  }

  void
  AddTransform(const std::function<std::unique_ptr<Transform<T>>()> &creater) {
    std::unordered_set<uint64_t> new_leaves_;
    for (auto &each : leaves_) {
      auto transform = creater();
      uint64_t new_id = transform->GetId();
      node_check(new_id);
      assert(nodes_.find(each) != nodes_.end());
      connect(*nodes_[each], *transform);
      new_leaves_.insert(new_id);
      nodes_.insert({new_id, std::move(transform)});
    }
    leaves_ = new_leaves_;
  }

  void SetSink(std::unique_ptr<Sink<T>> &&sink) {
    if (leaves_.empty()) {
      throw std::runtime_error("can not set sink for empty_leaves pipeline");
    }
    uint64_t id = sink->GetId();
    node_check(id);
    for (auto &each : leaves_) {
      connect(*nodes_[each], *sink);
    }
    leaves_.clear();
    leaves_.insert(id);
    nodes_.insert({id, std::move(sink)});
    completed_ = true;
  }

  Lazy<void> Run() && {
    if (completed_ == false) {
      throw std::runtime_error("try to run incomplete pipeline");
    }
    std::vector<async_simple::coro::RescheduleLazy<void>> lazies;
    async_simple::Executor *ex = co_await async_simple::CurrentExecutor{};
    for (auto &&node : nodes_) {
      lazies.emplace_back(std::move(node.second)->work().via(ex));
    }
    co_await async_simple::coro::collectAllPara(std::move(lazies));
    co_return;
  }

private:
  std::unordered_map<uint64_t, std::unique_ptr<Processor<T>>> nodes_;
  std::unordered_set<uint64_t> leaves_;
  bool completed_;

  void node_check(uint64_t id) {
    if (nodes_.find(id) != nodes_.end()) {
      throw std::runtime_error("add duplicate nodes to pipeline");
    }
  }
};

} // namespace tunnel