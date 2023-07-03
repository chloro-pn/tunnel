#include <functional>
#include <iostream>
#include <string>

#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "tunnel/channel_sink.h"
#include "tunnel/channel_source.h"
#include "tunnel/pipeline.h"
#include "tunnel/sink.h"
#include "tunnel/source.h"

using namespace tunnel;

class MySink : public Sink<std::string> {
 public:
  virtual async_simple::coro::Lazy<void> consume(std::string &&value) override {
    std::cout << value << std::endl;
    co_return;
  }
};

class MySource : public Source<std::string> {
 public:
  virtual async_simple::coro::Lazy<std::optional<std::string>> generate() override {
    if (eof == false) {
      eof = true;
      co_return std::string("hello world");
    }
    co_return std::optional<std::string>{};
  }
  bool eof = false;
};

class MyTransform : public Transform<std::string> {
 public:
  virtual async_simple::coro::Lazy<void> work() override {
    auto input_channel = this->GetInputPort();
    auto output_channel = this->GetOutputPort();
    Pipeline<std::string> sub_pipeline;
    auto source = std::make_unique<ChannelSource<std::string>>();
    source->SetInputChannel(input_channel);
    auto id = sub_pipeline.AddSource(std::move(source));

    sub_pipeline.ForkFrom(id, 2);

    auto sink = std::make_unique<ChannelSink<std::string>>();
    sink->SetOutputChannel(output_channel);
    sub_pipeline.SetSink(std::move(sink));
    auto result = co_await std::move(sub_pipeline).Run();
    // handle result
  }
};

int main() {
  Pipeline<std::string> pipe;
  auto id = pipe.AddSource(std::make_unique<MySource>());
  pipe.AddTransform(id, std::make_unique<MyTransform>());
  pipe.SetSink(std::make_unique<MySink>());
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipe).Run().via(&ex));
  return 0;
}
