#ifndef TUNNEL_PIPELINE_H
#define TUNNEL_PIPELINE_H

#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "async_simple/Executor.h"
#include "async_simple/coro/Collect.h"
#include "tunnel/concat.h"
#include "tunnel/dispatch.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

namespace tunnel {

template <typename T>
class Pipeline {
 public:
  Pipeline() {}

  uint64_t AddSource(std::unique_ptr<Source<T>>&& source) {
    uint64_t id = source->GetId();
    new_node_check(id);
    nodes_.insert({id, std::move(source)});
    leaves_.insert(id);
    return id;
  }

  uint64_t AddTransform(uint64_t leaf_id, std::unique_ptr<Transform<T>>&& transform) {
    uint64_t trans_id = transform->GetId();
    new_node_check(trans_id);
    leaf_check(leaf_id);
    assert(nodes_.find(leaf_id) != nodes_.end());
    connect(*nodes_[leaf_id], *transform);
    leaves_.erase(leaf_id);
    leaves_.insert(trans_id);
    nodes_.insert({trans_id, std::move(transform)});
    return trans_id;
  }

  std::unordered_set<uint64_t> AddTransform(const std::function<std::unique_ptr<Transform<T>>()>& creater) {
    std::unordered_set<uint64_t> new_leaves_;
    for (auto& each : leaves_) {
      auto transform = creater();
      uint64_t new_id = transform->GetId();
      new_node_check(new_id);
      assert(nodes_.find(each) != nodes_.end());
      connect(*nodes_[each], *transform);
      new_leaves_.insert(new_id);
      nodes_.insert({new_id, std::move(transform)});
    }
    leaves_ = new_leaves_;
    return new_leaves_;
  }

  uint64_t SetSink(std::unique_ptr<Sink<T>>&& sink) { return merge<false>(std::move(sink)); }

  uint64_t SetSink(std::unique_ptr<Sink<T>>&& sink, const std::unordered_set<uint64_t>& leaves) {
    return merge_to<false>(std::move(sink), leaves);
  }

  uint64_t Merge(std::unique_ptr<Transform<T>>&& transform, const std::unordered_set<uint64_t>& leaves) {
    return merge_to<true>(std::move(transform), leaves);
  }

  uint64_t Merge(const std::unordered_set<uint64_t>& leaves) {
    auto no_op = std::make_unique<NoOpTransform<T>>();
    return merge_to<true>(std::move(no_op), leaves);
  }

  uint64_t Merge(std::unique_ptr<Transform<T>>&& transform) { return merge<true>(std::move(transform)); }

  uint64_t Merge() {
    auto no_op = std::make_unique<NoOpTransform<T>>();
    return merge<true>(std::move(no_op));
  }

  std::unordered_set<uint64_t> DispatchTo(uint64_t leaf, std::unique_ptr<Dispatch<T>>&& node) {
    leaf_check(leaf);
    std::unordered_set<uint64_t> result;
    size_t new_size = node->GetSize();
    for (size_t i = 0; i < new_size; ++i) {
      auto no_op = std::make_unique<NoOpTransform<T>>();
      connect(*node, *no_op);
      uint64_t noop_id = no_op->GetId();
      result.insert(noop_id);
      nodes_.insert({noop_id, std::move(no_op)});
    }
    connect(*nodes_[leaf], *node);
    leaves_.erase(leaf);
    for (auto& each : result) {
      leaves_.insert(each);
    }
    uint64_t dispatch_id = node->GetId();
    nodes_.insert({dispatch_id, std::move(node)});
    return result;
  }

  uint64_t ConcatFor(const std::vector<uint64_t>& leaves) {
    leaves_check(leaves);
    auto concat_node = std::make_unique<Concat<T>>();
    for (auto& each : leaves) {
      connect(*nodes_[each], *concat_node);
      leaves_.erase(each);
    }
    uint64_t id = concat_node->GetId();
    leaves_.insert(id);
    nodes_.insert({id, std::move(concat_node)});
    return id;
  }

  Lazy<void> Run() && {
    if (leaves_.empty() == false) {
      throw std::runtime_error("try to run incomplete pipeline");
    }
    std::vector<async_simple::coro::RescheduleLazy<void>> lazies;
    async_simple::Executor* ex = co_await async_simple::CurrentExecutor{};
    if (ex == nullptr) {
      throw std::runtime_error("pipeline must be run with executor");
    }
    for (auto&& node : nodes_) {
      lazies.emplace_back(std::move(node.second)->work().via(ex));
    }
    co_await async_simple::coro::collectAllPara(std::move(lazies));
    co_return;
  }

  bool IsCompleted() const {
    return leaves_.empty();
  }

  const std::unordered_set<uint64_t>& CurrentLeaves() const {
    return leaves_;
  }

 private:
  std::unordered_map<uint64_t, std::unique_ptr<Processor<T>>> nodes_;
  std::unordered_set<uint64_t> leaves_;

  void new_node_check(uint64_t id) {
    if (nodes_.find(id) != nodes_.end()) {
      throw std::runtime_error("add duplicate node to pipeline");
    }
  }

  void leaf_check(uint64_t leaf_id) {
    if (leaves_.find(leaf_id) == leaves_.end()) {
      throw std::runtime_error("invalid leaf node for pipeline");
    }
  }

  void leaves_check(const std::unordered_set<uint64_t>& leaves) {
    for (auto& each : leaves) {
      leaf_check(each);
    }
  }

  void leaves_check(const std::vector<uint64_t>& leaves) {
    for (auto& each : leaves) {
      leaf_check(each);
    }
  }

  template <bool insert_to_leaves>
  uint64_t merge_to(std::unique_ptr<Processor<T>>&& node, const std::unordered_set<uint64_t>& leaves) {
    uint64_t id = node->GetId();
    new_node_check(id);
    leaves_check(leaves);
    auto queue = std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size);
    for (auto& each : leaves) {
      connect(*nodes_[each], *node, queue);
      leaves_.erase(each);
    }
    nodes_.insert({id, std::move(node)});
    if constexpr (insert_to_leaves) {
      leaves_.insert(id);
    }
    return id;
  }

  template <bool insert_to_leaves>
  uint64_t merge(std::unique_ptr<Processor<T>>&& node) {
    if (leaves_.empty()) {
      throw std::runtime_error("can not merge for empty_leaves pipeline");
    }
    uint64_t id = node->GetId();
    new_node_check(id);
    auto queue = std::make_shared<BoundedQueue<std::optional<T>>>(default_channel_size);
    for (auto& each : leaves_) {
      connect(*nodes_[each], *node, queue);
    }
    leaves_.clear();
    nodes_.insert({id, std::move(node)});
    if constexpr (insert_to_leaves) {
      leaves_.insert(id);
    }
    return id;
  }
};

}  // namespace tunnel

#endif
