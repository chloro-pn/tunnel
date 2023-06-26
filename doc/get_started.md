## hello world

首先我们来构建并执行一个最简单的`pipeline`，它只有一个`Source`节点和一个`Sink`节点，`Source`节点产生一条`"hello world"`的`string`，`Sink`节点消费它并打印到标准输出。

```c++
#include <iostream>
#include <string>
#include "tunnel/sink.h"

using namespace tunnel;

class MySink : public Sink<std::string> {
 public:
  virtual async_simple::coro::Lazy<void> consume(std::string &&value) override {
    std::cout << value << std::endl;
    co_return;
  }
};
```
本项目中每种`Processor`节点都具有一个模板参数，这个模板参数表示节点间传递数据的类型。自定义`Sink`节点，我们需要继承`tunnel::Sink<T>`，实现虚函数`virtual async_simple::coro::Lazy<void> consume(T &&value)`

下面是自定义`Source`节点： 

```c++
#include "tunnel/source.h"

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
```
自定义`Source`节点，我们需要继承`tunnel::Source<T>`，实现虚函数`virtual async_simple::coro::Lazy<std::optional<T>> generate()`。当你需要终止生产数据，记得要返回一个空的`std::optional<T>`表示结束。

```c++
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "tunnel/pipeline.h"

int main() {
  Pipeline<std::string> pipe;
  pipe.AddSource(std::make_unique<MySource>());
  pipe.SetSink(std::make_unique<MySink>());
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipe).Run().via(&ex));
  return 0;
}
```

最后，我们定义一个`Pipeline<std::string>`，通过`AddSource`和`SetSink`接口分别为pipeline设置`Source`和`Sink`节点，并通过`Run`接口启动它。这里使用`async_simple::coro::syncAwait`同步等待pipeline执行完毕，你将会在标准输出中看到`hello world`。