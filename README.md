![tunnel icon](https://github.com/chloro-pn/draw_io_repo/blob/master/tunnel.svg)
## Tunnel | [中文](./README_CN.md)

![](https://tokei.rs/b1/github/chloro-pn/tunnel) ![](https://tokei.rs/b1/github/chloro-pn/tunnel?category=files) ![Static Badge](https://img.shields.io/badge/c%2B%2B-20-blue)

Tunnel is a cross platform, lightweight, and highly adaptable task execution framework based on `C++20 coroutine`. You can use it to build task execution engines with complex dependencies, or pipeline execution engines.The idea of this project comes from the execution engine of `ClickHouse`. 

This project has the following features:

* The user's processing logic does not need to focus on scheduling, synchronization, or mutual exclusion. You only need to design a reasonable DAG structure to achieve the ability of **multi-core parallel execution**;

* Thanks to the powerful customization capabilities of c++20 coroutine, you can **easily integrate with other asynchronous systems or network io** (which means that `tunnel` can be easily expanded into a distributed task execution framework, which is also one of the long-term goals of this project);

* Thanks to async_simple with good design and interface, you can **control which Executor each node in the Pipeline is scheduled on**, which is beneficial for resource isolation and management;

* Supports **passing parameters between nodes**, although each `Pipeline` only supports one parameter type. If you need to pass different types of data between different nodes, please use `std::any` or `void *` and perform runtime conversion;

## Compiler Requirement
* This project is based on the c++20 standard.
* This project uses `bazel` to build the system.
* This project is based on `async_simple`, so first ensure that your compiler (`clang`, `gcc`, `Apple clang`) supports compiling `async_simple`.
* This project supports `MacOS`, `Linux`, and `Windows `operating systems.

## Dependencies
* async_simple
* googletest
* chriskohlhoff/asio
* rigtorp/MPMCQueue
* gflags
* spdlog

## Design
Firstly, you need to understand several basic concepts:

* **`Processor`**: `Processor` is the basic unit for scheduling execution, and each `Processor` holds 0, 1, or more `input_port` and 0, 1, or more `output_port`. But it will not hold 0 `input_port` and 0 `output_port` at the same time.

* **`port`**：`port` is a tool for transferring data between `Processor`, and some `ports` share the same queue. `port` and are divided into `input_port` and `output_port`, `input_port` reads data from the queue, and `output_port` writes data to the queue.

* **`pipeline`**：a `pipeline` is composed of multiple `processors`. These `processors` are connected through queue and have the structure of a Directed Acyclic Graph. The `pipeline` can be sent to the `Executor` for scheduling and execution.

* **`Executor`**：the `Executor` concept in `async_simple`.

The above are the four most basic concepts in this project, followed by some derived concepts:
* `Source`：`Source` is a type of `Processor` that does not have an `input_port` and is the node that generates data.
* `EmptySource`：`EmptySource` is a type of `Source` that only generates a EOF info.
* `ChannelSource`：`ChannelSource`is a type of `Source` that read data from bind_channel.
* `Sink`：`Sink` is a type of `Processor` that does not have an `output_port` and is a node that consumes data.
* `DumpSimk`：`DumpSimk` is a type of `Sink` that reads and discards data.
* `ChannelSink`：`ChannelSink`is a type of `Sink` that read data and write to bind_channel.
* `TransForm`：`TransForm` is a type of `Processor` that exists only to provide a different `Processor` type from `Source` and `Sink`.
* `SimpleTransForm`：`SimpleTransForm` is a type of `TransForm` that only has one `input_port` and one `output_port`, used to perform simple transformations. Most of the user's logic should be accomplished through inheritance of this class.
* `NoOpTransform`：`NoOpTransform` is a type of `SimpleTransForm` that is only used for placeholders.
* `Concat`：`Concat` is a type of `Processor` that has one or more `input_ports` and one `output_port`, and it can be used for sequential consumption.
* `Dispatch`：`Dispatch` is a type of `Processor` that has one `input_port` and one or more `output_ports`, and it can be used for division.
* `Filter`：`Filter` is a type of `TransForm` that can be used for filtering.
* `Accumulate`：`Accumulate` is a type of `TransForm` that can be used for accumulation.
* `Fork`：`Fork` is a type of `Processor` that has one `input_port` and one or more `output_port`, it can be used for replication.

**NOTE**：This project does not have a `Merge` node, but implements the `Merge` function through other methods. The reason is that the `Merge` node requires multiple `input_ports`, but we cannot know which `input_port` currently has data coming, so we need to suspend waiting for a certain `input_port`, which is unreasonable. This project achieves this function by sharing multiple `port` queues, as detailed in the `Merge` interface of the `Pipeline`.

---
The inheritance relationship of node types is as follows. Types marked in **red** indicate the need for inheritance implementation, while types marked in **blue** indicate that they can be directly used：

---
![node_type](https://github.com/chloro-pn/draw_io_repo/blob/master/nodes.drawio.svg)

## Doc

**hello world**

here is a Hello World program:
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

As you can see, users need to inherit some Processors to implement custom processing logic, then combine these Processors in a certain structure through Pipeline, and finally start executing the Pipeline.

For example, for the Source node, only the generate() method needs to be rewritten to generate data. Users need to ensure that an empty `std::optional<T>{}` representing EOF information is ultimately returned, otherwise the Pipeline will not stop executing; For Sink nodes, the consume() method needs to be rewritten to consume data.

For the use of more Processor types, users can read the source files in the tunnel directory.

**about exception**

If a Processor throws an exception during the pipeline running, tunnel may call `std::abort` to abort the process (`bind_abort_channel == false`), or catch the exception and pass the exit information to other Processors. The Processor receiving the exit information will enter managed mode, and user logic will not be called again in managed mode. It simply reads data from upstream and discards it, After all upstream data is read, EOF information is written to downstream and execution ends.

**about expand pipeline at runtime**

Users can construct and run a new pipeline in the Processor's processing logic, and connect the data streams of two pipelines through `ChannelSource` and `ChannelSink`. This feature is useful in certain situations, such as when you need to decide how to handle the remaining data based on the data generated during the pipeline execution process.

There is a simple example in example/embedpipeline.cc.

**about pipeline interface**

tunnel will assign a unique ID to each Processor instance, through which users and tunnel exchange pipeline structure information.

The API of pipeline follows the principle of only allowing post nodes to be added to leaf nodes. Leaf nodes refer to non-sink nodes that have not yet specified an output_port, for example, there is an empty pipeline:
* Firstly, add a source node (id == 1) through AddSource, so there is only one leaf node 1 in the pipeline.
* Then, by using AddTransform, add a post transform node (id == 2) to the source node , and the current leaf node in the pipeline will become 2.
* Next, add another source node (id == 3) through AddSource, so there are two leaf nodes in the pipeline now, 2 and 3.
* Finally, add a shared sink post node (id == 4) to all current leaf nodes ( 2 and 3 ) through SetSink. At this point, no leaf nodes exist in the pipeline. A pipeline without leaf nodes is called complete, and only complete pipelines can be executed.

Please read the content in the doc directory and example directory to learn about the API usage of this project.

## Todo
1. Support for more types of nodes [**doing**]
2. Support Pipeline Merge [done]
3. Topology detection
4. Schedule event collection [**doing**]
5. Support active stop of execution [done with throw exception]
6. Exception handling during execution [done]
7. Implementing a high-performance Executor [done]
8. Support for extension of Pipeline at runtime [done]
9. Support for distributed scheduling (support for network io based on async_simple first)

## License
tunnel is distributed under the Apache License (Version 2.0).
