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

#ifndef TUNNEL_PROCESSOR_H
#define TUNNEL_PROCESSOR_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include "tunnel/channel.h"

namespace tunnel {

namespace detail {

static uint64_t GenerateId() {
  static std::atomic<uint64_t> id_{1};
  return id_.fetch_add(1);
}
}  // namespace detail

constexpr size_t max_yeild_count = 10;

template <typename T>
class Pipeline;

/*
 * Processor is the basic scheduling unit in the pipeline. Each instance of Processor
 * will be assigned a unique ID. In addition, users can specify a name for it to
 * facilitate identifying different Processors when print pipeline's structure information.
 *
 * a Processor holds an abort_channel, which is shared by all Processors on the same pipeline.
 * When the Processor exits early due to catch an exception, it notifies other Processor to exit as soon as possible
 * through the abort_channel.
 */
template <typename T>
class Processor {
 protected:
  friend class Pipeline<T>;

  explicit Processor(const std::string &name = "")
      : processor_id_(detail::GenerateId()), name_(name), input_count_(0) {}

 private:
  virtual async_simple::coro::Lazy<void> work() { throw std::runtime_error("work function is not implemented"); }

  // after co_await work() return with an exception, Processor will first notify other nodes through the abort channel,
  // then enter the hosted mode by co_await this lazy
  virtual async_simple::coro::Lazy<void> hosted_mode() { co_return; }

  // co_await work() and handle exception
  async_simple::coro::Lazy<void> work_with_exception() {
    try {
      before_work();
    } catch (const std::exception &e) {
      std::cerr << "node " << GetId() << " : " << GetName() << " throw exception before work : " << e.what()
                << std::endl;
      std::abort();
    }
    async_simple::Try<void> result = co_await work().coAwaitTry();
    if (result.hasError()) {
      std::string exception_msg;
      if (!abort_port) {
        try {
          std::rethrow_exception(result.getException());
        } catch (const std::exception &e) {
          exception_msg = e.what();
        }
        std::cerr << "node " << GetId() << " : " << GetName()
                  << " throw exception and abort, we can't handle exception because it doesn't bind abort channel : "
                  << std::endl
                  << exception_msg << std::endl;
        std::abort();
      }
      // We only need to TryPush to notify the exit information
      co_await abort_port.GetQueue().TryPush(0);
      co_await hosted_mode();
      std::rethrow_exception(result.getException());
    }
    // after_work will be called without exception occur
    else {
      co_await after_work();
    }
    co_return;
  }

 protected:
  async_simple::coro::Lazy<std::optional<T>> Pop(Channel<T> &input, size_t &input_count) {
    while (true) {
      std::optional<T> value = co_await input.GetQueue().Pop();
      bool is_eof = !value.has_value();
      bool should_return = false;
      if (is_eof) {
        assert(input_count > 0);
        --input_count;
        if (input_count == 0) {
          // write EOF information back into input so that other nodes reading this queue can also read EOF information
          co_await Push(std::optional<T>{}, input);
          input.reset();
          should_return = true;
        }
      } else {
        should_return = true;
      }
      // if it is the last EOF information, the abort_port should not be checked, otherwise it may cause the output
      // channel to lose EOF information.
      if (should_return && is_eof) {
        co_return value;
      }
      if (abort_port) {
        async_simple::Try<std::optional<int>> abort_info = co_await abort_port.GetQueue().TryPop();
        if (abort_info.available()) {
          throw std::runtime_error("throw by abort channel");
        }
      }
      if (should_return) {
        co_return value;
      }
    }
  }

  async_simple::coro::Lazy<void> Push(std::optional<T> &&v, Channel<T> &output) {
    bool is_eof = !v.has_value();
    co_await output.GetQueue().Push(std::move(v));
    if (is_eof) {
      output.reset();
    }
    if (abort_port) {
      async_simple::Try<std::optional<int>> abort_info = co_await abort_port.GetQueue().TryPop();
      if (abort_info.available()) {
        throw std::runtime_error("throw by abort channel");
      }
    }
    co_return;
  }

  // perform some checks before co_await work(), you can throw an exception to terminate execution
  virtual void before_work() {}

  // be called after co_await work(), look at channel_sink for detail.
  virtual async_simple::coro::Lazy<void> after_work() { co_return; }

  async_simple::coro::Lazy<void> close_input(Channel<T> &input, size_t &input_count) {
    if (input) {
      while (true) {
        std::optional<T> value = co_await input.GetQueue().Pop();
        if (!value.has_value()) {
          assert(input_count_ > 0);
          --input_count_;
          if (input_count_ == 0) {
            co_return;
          }
        }
      }
    }
    co_return;
  }

  async_simple::coro::Lazy<void> close_output(Channel<T> &output) {
    if (output) {
      co_await output.GetQueue().Push(std::optional<T>{});
    }
    co_return;
  }

 private:
  // can only be called by Pipeline.
  void BindAbortChannel(const Channel<int> &abort) { abort_port = abort; }

 public:
  virtual ~Processor() {}

  void SetInputPort(const Channel<T> &input) {
    input_port = input;
    ++input_count_;
  }

  void SetOutputPort(const Channel<T> &output) { output_port = output; }

  uint64_t &GetId() noexcept { return processor_id_; }

  size_t GetInputCount() const { return input_count_; }

  const Channel<T> &GetInputPort() const { return input_port; }

  Channel<T> &GetInputPort() { return input_port; }

  const Channel<T> &GetOutputPort() const { return output_port; }

  Channel<T> &GetOutputPort() { return output_port; }

  const Channel<T> &GetAbortChannel() const { return abort_port; };

  Channel<T> &GetAbortChannel() { return abort_port; };

  const std::string &GetName() const { return name_; }

 private:
  uint64_t processor_id_;
  std::string name_;
  // This is a channel used to abort execution, set by the pipeline for each Processor before scheduling.
  // All nodes share the same abort_channel. When the Processor throws an exception, the tunnel will catch the
  // exception and notify other Processors. If we read some data from abort_channel, we should co_return as soon as
  // possible.
  Channel<int> abort_port;

 protected:
  Channel<T> input_port;
  Channel<T> output_port;
  // input_channel will be written by input_count_ Processors, so it needs to read input_count_ EOF information to
  // complete the reading.
  size_t input_count_;
};

template <typename T>
void connect(Processor<T> &input, Processor<T> &output, size_t channel_size) {
  Channel<T> channel(std::make_shared<BoundedQueue<std::optional<T>>>(channel_size));
  input.SetOutputPort(channel);
  output.SetInputPort(channel);
}

template <typename T>
void connect(Processor<T> &input, Processor<T> &output, std::shared_ptr<BoundedQueue<std::optional<T>>> &queue) {
  Channel<T> channel(queue);
  input.SetOutputPort(channel);
  output.SetInputPort(channel);
}

}  // namespace tunnel

#endif
