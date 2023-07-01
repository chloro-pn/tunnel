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

## Design
首先理解几个基本概念：
* **`Processor`**：`Processor`是调度执行的基本单位，每个`Processor`会持有0个、1个或者多个`input_port`，以及0个、1个或者多个`output_port`。但是不会同时持有0个`input_port`和`output_port`（孤立节点没有意义）。
* **`port`**：`port`是`Processor`间传递数据的工具，不同的`port`间共享一条队列，`port`分为`input_port`和`output_port`，`input_port`从队列中读取数据，`output_port`向队列中写入数据。
* **`pipeline`**：一个`pipeline`由多个`Processor`组成，这些`Processor`通过`port`连接，具有有向无环图的结构，`pipeline`可以被交由`Executor`调度执行。
* **`Executor`**：`async_simple`中的`Executor`。

以上是本项目中最基本的四个概念，接下来是一些派生概念：
* `Source`：`Source`是一种`Processor`，它不具有`input_port`，是产生数据的节点。
* `EmptySource`：`Source`是一种`Processor`，它只会产生一个EOF数据。
* `Sink`：`Sink`是一种`Processor`，它不具有`output_port`，是消费数据的节点。
* `DumpSimk`：`DumpSimk`是一种`Sink`，它读取并丢弃数据。
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
请阅读doc目录和example目录学习本项目的api使用。

## Todo
1. 支持更多类型的节点 [**doing**]
2. 支持Pipeline合并 [done]
3. 拓扑检测
4. 调度事件收集 [**doing**]
5. 支持中止执行 [done with throw exception]
6. 执行过程中的异常处理 [done]
7. 实现一个高性能的Executor [**doing**]
8. 支持运行时扩展Pipeline
9. 支持分布式调度（首先需要支持基于coroutine的网络io）


## License
本项目基于Apache License (Version 2.0) 协议。
