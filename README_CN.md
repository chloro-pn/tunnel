![tunnel icon](https://github.com/chloro-pn/draw_io_repo/blob/master/tunnel.svg)
## Tunnel

![](https://tokei.rs/b1/github/chloro-pn/tunnel) ![](https://tokei.rs/b1/github/chloro-pn/tunnel?category=files) ![Static Badge](https://img.shields.io/badge/c%2B%2B-20-blue)

Tunnel是一个跨平台、轻量级、适配性强的基于c++20 coroutine的任务执行框架，你可以用它构建具有复杂依赖关系的task执行引擎，或者pipeline执行引擎。本项目的思路来源于ClickHouse的执行引擎。

本项目有以下特点：
* 用户的处理逻辑不需要关注调度、同步、互斥，你仅需要设计合理的DAG结构就可以获得多核并行执行的能力；
* 得益于c++20 coroutine强大的定制能力，你可以方便的与其他异步系统或者网络io集成（这意味着tunnel可以比较容易的扩展为分布式任务执行框架，这也是本项目的长期目标之一）；
* 得益于async_simple良好的设计与接口，你可以控制Pipeline中的任何节点在哪个Executor上调度，这有利于资源隔离和管理；
* 支持在节点间传递参数，尽管每个Pipeline只支持一种参数类型，如果您需要在不同的节点间传递不同类型的数据，请使用std::any或者void*并执行运行时转换；

## Compiler Requirement
* 本项目基于c++20标准。
* 本项目使用bazel构建系统。
* 本项目基于async_simple实现，因此首先确保您的编译器（clang、gcc、Apple-clang）版本支持编译async_simple。
* 本项目支持MacOS、Linux和Windows操作系统。

## Dependencies
* async_simple
* googletest
* chriskohlhoff/asio
* rigtorp/MPMCQueue
* gflags
* spdlog

## Design
首先理解几个基本概念：
* **`Processor`**：`Processor`是调度执行的基本单位，每个`Processor`会持有0个、1个或者多个`input_port`，以及0个、1个或者多个`output_port`。但是不会同时持有0个`input_port`和`output_port`（孤立节点没有意义）。
* **`port`**：`port`是`Processor`间传递数据的工具，不同的`port`间共享一条队列，`port`分为`input_port`和`output_port`，`input_port`从队列中读取数据，`output_port`向队列中写入数据。
* **`pipeline`**：一个`pipeline`由多个`Processor`组成，这些`Processor`通过队列连接，具有有向无环图的结构，`pipeline`可以被交由`Executor`调度执行。
* **`Executor`**：`async_simple`中的`Executor`。

以上是本项目中最基本的四个概念，接下来是一些派生概念：
* `Source`：`Source`是一种`Processor`，它不具有`input_port`，是产生数据的节点。
* `EmptySource`：`EmptySource`是一种`Source`，它只会产生一个EOF数据。
* `ChannelSource`：`ChannelSource`是一种`Source`，它会从绑定的channel中读取数据。
* `Sink`：`Sink`是一种`Processor`，它不具有`output_port`，是消费数据的节点。
* `DumpSimk`：`DumpSimk`是一种`Sink`，它读取并丢弃数据。
* `ChannelSink`：`ChannelSink`是一种`Sink`，它将读取的数据写入绑定的channel中。
* `TransForm`：`TransForm`是一种`Processor`，它的存在仅为了提供一种不同于`Source`和`Sink`的`Processor`类型。
* `SimpleTransForm`：`SimpleTransForm`是一种`TransForm`，它只具有一个`input_port`和一个`output_port`，用于执行简单的转换，用户的大部分逻辑应该通过继承此类完成。
* `NoOpTransform`：`NoOpTransform`是一种`SimpleTransForm`，它具有占位功能。
* `Concat`：`Concat`是一种`Processor`，它具有一个或多个`input_port`和一个`output_port`，它具有顺序消费功能。
* `Dispatch`：`Dispatch`是一种`Processor`，它具有一个`input_port`和一个或多个`output_port`，它具有分流功能。
* `Filter`：`Filter`是一种`TransForm`，它具有过滤功能。
* `Accumulate`：`Accumulate`是一种`TransForm`，它具有累计功能。
* `Fork`：`Fork`是一种`Processor`，它具有一个`input_port`和一个或多个`output_port`，它具有复制功能。

**注意**：本项目没有`Merge`节点，而是通过其他方法实现`Merge`功能，原因是`Merge`节点需要多个`input_port`，但是我们没有办法知道哪个`input_port`当前有数据到来，因此需要挂起等待某个`input_port`，这是不合理的。本项目通过共享多个`port`的队列来满足此功能，详情见`pipeline`的`Merge`接口。

---
节点类型的继承关系如下，标为红色的类型表示需要继承实现，标为蓝色的类型表示可以直接使用：
![node_type](https://github.com/chloro-pn/draw_io_repo/blob/master/nodes.drawio.svg)

## Doc

**hello world**

以下是一个Hello World程序：
```c++
#include <functional>
#include <iostream>
#include <string>

#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
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
  async_simple::executors::SimpleExecutor ex(2);
  async_simple::coro::syncAwait(std::move(pipe).Run().via(&ex));
  return 0;
}
```
如你所见，用户需要继承部分Processor以实现自定义处理逻辑，然后通过Pipeline将这些Processor按照某种结构组合起来，最后通过Pipeline的Run函数开始执行。
例如，对于Source节点，只需要重写generate()方法来产生数据，用户需要确保最终会返回一个空的optional表示EOF信息，否则Pipeline会一直执行；对于Sink节点，需要重写consume()方法来消费数据。
对于更多Processor类型的使用，用户可以自行阅读tunnel目录下的源文件，

**about exception**

如果在pipeline运行过程中Processor抛出异常，根据构造Pipeline时传递的参数，tunnel可能会调用std::abort中止进程（`bind_abort_channel == false`)，或者捕获异常并将退出信息传递给其他Processor，接收到退出信息的Processor会进入托管模式，托管模式下不会再调用用户逻辑，只是简单的从上游读取数据并丢弃，当所有上游数据读取完毕后，向下游写入EOF信息，最后结束执行。

**about expand pipeline at runtime**

用户可以在Processor的处理逻辑中构造并调度一个新的Pipeline，并且可以通过ChannelSource和ChannelSink连通两个pipeline的数据流。在某些情况下这个特性很有用，例如你需要根据pipeline执行过程中产生的一些数据来决定如何处理剩余的数据。
example/embed_pipeline.cc中有一个简单的例子。


**about pipeline interface**

tunnel会为每个Processor实例分配一个唯一的id，用户与tunnel通过此id交换pipeline的结构信息。
pipeline的api遵循这样的原则：只允许为叶节点添加后置节点。叶节点指的是还没有指定output_port的pipeline中的非sink节点，例如对于一个空的pipeline：
* 首先通过AddSource添加一个source节点，它的id为1，那么pipeline中只有一个叶节点1。
* 然后通过AddTransform为source节点添加一个transform后置节点，它的id为2，那么pipeline中现在的叶节点变为2。
* 接着通过AddSource添加另一个source节点，它的id为3，那么pipeline中现在有两个叶节点，2和3。
* 最后通过SetSink为当前所有的叶节点添加一个共享的sink后置节点, 它的id为4，此时pipeline中不存在叶节点。不存在叶节点的pipeline被称为完整的，只有完整的pipeline才可以被执行。

请阅读doc目录和example目录学习本项目的api使用。

## Todo
1. 支持更多类型的节点 [**doing**]
2. 支持Pipeline合并 [done]
3. 拓扑检测
4. 调度事件收集 [**doing**]
5. 支持中止执行 [done with throw exception]
6. 执行过程中的异常处理 [done]
7. 实现一个高性能的Executor [done]
8. 支持运行时扩展Pipeline [done]
9. 支持分布式调度（首先需要支持基于coroutine的网络io）


## License
本项目基于Apache License (Version 2.0) 协议。
