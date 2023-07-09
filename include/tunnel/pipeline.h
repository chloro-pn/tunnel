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

#ifndef TUNNEL_PIPELINE_H
#define TUNNEL_PIPELINE_H

#include <cassert>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "async_simple/Executor.h"
#include "async_simple/coro/Collect.h"
#include "tunnel/concat.h"
#include "tunnel/dispatch.h"
#include "tunnel/fork.h"
#include "tunnel/simple_transform.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

namespace tunnel {

struct PipelineOption {
  // 是否为Processor节点绑定abort_channel。
  // 当节点抛出异常时：
  // * 如果不绑定abort_channel则会调用std::abort终止程序；
  // * 如果绑定abort_channel，则抛出异常的节点会将退出信息传递给其他节点，当其他节点通过abort_channel读到退出信息后也会抛出异常。
  //   节点抛的异常会被底层的调度协程捕获，进入托管模式，该模式下节点尝试读取input_channel的数据直到读到EOF信息，然后向output_channel写入EOF信息并退出。
  bool bind_abort_channel = false;
  // Processor节点间队列的容量。
  // MergePipeline后这个值会被设置为0，表示新的Pipeline中有多种容量的队列
  size_t channel_size = default_channel_size;
  std::string name = "default";
};

template <typename T>
class Pipeline;

template <typename T>
Pipeline<T> MergePipeline(Pipeline<T>&& left, Pipeline<T>&& right);

template <typename T>
class Pipeline {
 public:
  friend Pipeline<T> MergePipeline<T>(Pipeline<T>&& left, Pipeline<T>&& right);

  explicit Pipeline(const PipelineOption& option = PipelineOption{}) : option_(option) {}

  uint64_t AddSource(std::unique_ptr<Source<T>>&& source) {
    uint64_t id = source->GetId();
    new_node_check(id);
    nodes_.insert({id, std::move(source)});
    leaves_.insert(id);
    sources_.push_back(id);
    return id;
  }

  uint64_t AddTransform(uint64_t leaf_id, std::unique_ptr<Transform<T>>&& transform) {
    uint64_t trans_id = transform->GetId();
    new_node_check(trans_id);
    leaf_check(leaf_id);
    assert(nodes_.find(leaf_id) != nodes_.end());
    connect(*nodes_[leaf_id], *transform, option_.channel_size);
    add_edge(leaf_id, trans_id);
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
      connect(*nodes_[each], *transform, option_.channel_size);
      add_edge(each, new_id);
      new_leaves_.insert(new_id);
      nodes_.insert({new_id, std::move(transform)});
    }
    leaves_ = new_leaves_;
    return new_leaves_;
  }

  uint64_t SetSink(std::unique_ptr<Sink<T>>&& sink) {
    auto id = merge<false>(std::move(sink));
    sinks_.push_back(id);
    return id;
  }

  uint64_t SetSink(std::unique_ptr<Sink<T>>&& sink, const std::unordered_set<uint64_t>& leaves) {
    auto id = merge_to<false>(std::move(sink), leaves);
    sinks_.push_back(id);
    return id;
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

  std::unordered_set<uint64_t> DispatchFrom(uint64_t leaf, std::unique_ptr<Dispatch<T>>&& node) {
    leaf_check(leaf);
    std::unordered_set<uint64_t> result;
    size_t new_size = node->Size();
    uint64_t dispatch_id = node->GetId();
    for (size_t i = 0; i < new_size; ++i) {
      auto no_op = std::make_unique<NoOpTransform<T>>();
      connect(*node, *no_op, option_.channel_size);
      uint64_t noop_id = no_op->GetId();
      add_edge(dispatch_id, noop_id);
      result.insert(noop_id);
      nodes_.insert({noop_id, std::move(no_op)});
    }
    connect(*nodes_[leaf], *node, option_.channel_size);
    add_edge(leaf, dispatch_id);
    leaves_.erase(leaf);
    for (auto& each : result) {
      leaves_.insert(each);
    }
    nodes_.insert({dispatch_id, std::move(node)});
    return result;
  }

  uint64_t ConcatFrom(const std::vector<uint64_t>& leaves) {
    leaves_check(leaves);
    auto concat_node = std::make_unique<Concat<T>>(leaves.size());
    uint64_t id = concat_node->GetId();
    for (auto& each : leaves) {
      connect(*nodes_[each], *concat_node, option_.channel_size);
      add_edge(each, id);
      leaves_.erase(each);
    }
    leaves_.insert(id);
    nodes_.insert({id, std::move(concat_node)});
    return id;
  }

  std::unordered_set<uint64_t> ForkFrom(uint64_t leaf, size_t size) {
    leaf_check(leaf);
    auto node = std::make_unique<Fork<T>>(size);
    uint64_t fork_id = node->GetId();
    std::unordered_set<uint64_t> result;
    for (size_t i = 0; i < size; ++i) {
      auto no_op = std::make_unique<NoOpTransform<T>>();
      connect(*node, *no_op, option_.channel_size);
      uint64_t noop_id = no_op->GetId();
      add_edge(fork_id, noop_id);
      result.insert(noop_id);
      nodes_.insert({noop_id, std::move(no_op)});
    }
    connect(*nodes_[leaf], *node, option_.channel_size);
    add_edge(leaf, fork_id);
    leaves_.erase(leaf);
    for (auto& each : result) {
      leaves_.insert(each);
    }
    nodes_.insert({fork_id, std::move(node)});
    return result;
  }

  Lazy<std::vector<async_simple::Try<void>>> Run() && {
    if (leaves_.empty() == false) {
      throw std::runtime_error("try to run incompleted pipeline");
    }
    if (nodes_.empty() == true) {
      throw std::runtime_error("try to run empty pipeline");
    }
    std::vector<async_simple::coro::RescheduleLazy<void>> lazies;
    async_simple::Executor* ex = co_await async_simple::CurrentExecutor{};
    if (ex == nullptr) {
      throw std::runtime_error("pipeline can not run without executor");
    }
    Channel<int> abort_channel(10);
    for (auto&& node : nodes_) {
      if (option_.bind_abort_channel) {
        node.second->BindAbortChannel(abort_channel);
      }
      lazies.emplace_back(std::move(node.second)->work_with_exception().via(ex));
    }
    auto results = co_await async_simple::coro::collectAllPara(std::move(lazies));
    co_return results;
  }

  bool IsCompleted() const {
    return leaves_.empty();
  }

  size_t GetProcessorSize() const { return nodes_.size(); }

  const std::unordered_set<uint64_t>& CurrentLeaves() const {
    return leaves_;
  }

  const std::vector<uint64_t>& GetSources() const { return sources_; }

  bool IsSource(uint64_t id) const {
    auto it = std::find(sources_.begin(), sources_.end(), id);
    return it != sources_.end();
  }

  const std::vector<uint64_t>& GetSinks() const { return sinks_; }

  bool IsSink(uint64_t id) const {
    auto it = std::find(sinks_.begin(), sinks_.end(), id);
    return it != sinks_.end();
  }

  const std::string& GetName() const { return option_.name; }

  std::string Dump() const {
    std::stringstream result;
    result << "pipeline : " << option_.name << "\n";
    result << "node size : " << nodes_.size() << "\n";
    for (auto& each : nodes_) {
      auto edges = dags_.find(each.first);
      result << each.first << " (" << each.second->GetName() << ") : ";
      if (edges != dags_.end()) {
        for (auto& to : edges->second) {
          result << to << " ";
        }
        result << "\n";
      } else {
        result << "\n";
      }
    }
    result << "the pipeline is " << (IsCompleted() ? "completed" : "incompleted") << "\n";
    return result.str();
  }

 private:
  PipelineOption option_;
  std::unordered_map<uint64_t, std::unique_ptr<Processor<T>>> nodes_;
  std::unordered_set<uint64_t> leaves_;
  std::unordered_map<uint64_t, std::unordered_set<uint64_t>> dags_;
  // record sources and sinks in the order of registration to support the merge of pipelines.
  std::vector<uint64_t> sources_;
  std::vector<uint64_t> sinks_;

  void add_edge(uint64_t from, uint64_t to) { dags_[from].insert(to); }

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
    auto queue = std::make_shared<BoundedQueue<std::optional<T>>>(option_.channel_size);
    for (auto& each : leaves) {
      connect(*nodes_[each], *node, queue);
      add_edge(each, id);
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
    auto queue = std::make_shared<BoundedQueue<std::optional<T>>>(option_.channel_size);
    for (auto& each : leaves_) {
      connect(*nodes_[each], *node, queue);
      add_edge(each, id);
    }
    leaves_.clear();
    nodes_.insert({id, std::move(node)});
    if constexpr (insert_to_leaves) {
      leaves_.insert(id);
    }
    return id;
  }
};

// these helper functions are used to pipeline merge
namespace detail {
inline void ReplaceSinkIdInDags(std::unordered_map<uint64_t, std::unordered_set<uint64_t>>& dags,
                                const std::vector<uint64_t>& old_sink_id, const std::vector<uint64_t>& new_id) {
  assert(old_sink_id.size() == new_id.size());
  for (size_t i = 0; i < old_sink_id.size(); ++i) {
    for (auto& each : dags) {
      if (each.second.find(old_sink_id[i]) != each.second.end()) {
        each.second.erase(old_sink_id[i]);
        each.second.insert(new_id[i]);
      }
    }
  }
}

inline void ReplaceSourceIdInDags(std::unordered_map<uint64_t, std::unordered_set<uint64_t>>& dags,
                                  const std::vector<uint64_t>& old_source_id, const std::vector<uint64_t>& new_id) {
  std::unordered_map<uint64_t, std::unordered_set<uint64_t>> add_dag;
  for (size_t i = 0; i < old_source_id.size(); ++i) {
    auto each = dags.begin();
    while (each != dags.end()) {
      if (each->first == old_source_id[i]) {
        add_dag.insert({new_id[i], std::move(each->second)});
        each = dags.erase(each);
      } else {
        ++each;
      }
    }
    for (auto&& each : add_dag) {
      dags.insert(std::move(each));
    }
  }
}
}  // namespace detail

template <typename T>
Pipeline<T> MergePipeline(Pipeline<T>&& left, Pipeline<T>&& right) {
  if (!left.IsCompleted() || !right.IsCompleted()) {
    throw std::runtime_error("can only merge two completed pipeline");
  }
  if (left.GetProcessorSize() == 0) {
    return std::move(right);
  }
  if (right.GetProcessorSize() == 0) {
    return std::move(left);
  }
  if (left.GetSinks().size() != right.GetSources().size()) {
    throw std::runtime_error("the number of sources and sinks in the merged pipeline is not equal");
  }
  std::vector<std::unique_ptr<Processor<T>>> noops;
  for (size_t i = 0; i < left.GetSinks().size(); ++i) {
    auto sink = std::move(left.nodes_[left.sinks_[i]]);
    auto source = std::move(right.nodes_[right.sources_[i]]);
    auto no_op = NoOpTransform<T>::MergeSinkAndSource(std::move(sink), std::move(source));
    noops.emplace_back(std::move(no_op));
  }

  Pipeline<T> new_pipeline;
  new_pipeline.option_.name = left.option_.name + "-merge-" + right.option_.name;
  new_pipeline.option_.bind_abort_channel = left.option_.bind_abort_channel && right.option_.bind_abort_channel;
  // 存在多种channel_size。
  new_pipeline.option_.channel_size = 0;
  for (auto&& left_node : left.nodes_) {
    if (!left.IsSink(left_node.first)) {
      new_pipeline.nodes_.insert(std::move(left_node));
    }
  }
  for (auto&& right_node : right.nodes_) {
    if (!right.IsSource(right_node.first)) {
      new_pipeline.nodes_.insert(std::move(right_node));
    }
  }
  // used for merge dags
  std::vector<uint64_t> noop_ids;
  for (auto&& noop_node : noops) {
    uint64_t id = noop_node->GetId();
    noop_ids.push_back(id);
    new_pipeline.nodes_.insert({id, std::move(noop_node)});
  }
  // also completed
  new_pipeline.leaves_ = {};
  // merge dags
  detail::ReplaceSinkIdInDags(left.dags_, left.sinks_, noop_ids);
  detail::ReplaceSourceIdInDags(right.dags_, right.sources_, noop_ids);
  new_pipeline.dags_ = std::move(left.dags_);
  for (auto&& each : right.dags_) {
    new_pipeline.dags_.insert(std::move(each));
  }
  // merge sources and sinks
  new_pipeline.sources_ = left.sources_;
  new_pipeline.sinks_ = right.sinks_;
  return new_pipeline;
}

}  // namespace tunnel

#endif
