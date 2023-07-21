#include <iostream>
#include <string>

#include "async_simple/coro/SyncAwait.h"
#include "executor/tunnel_executor.h"
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

int main() {
  Pipeline<std::string> pipe;
  pipe.AddSource(std::make_unique<MySource>());
  pipe.SetSink(std::make_unique<MySink>());
  tunnel::TunnelExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipe).Run().via(&ex));
  return 0;
}