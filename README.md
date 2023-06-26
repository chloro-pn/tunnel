![tunnel icon](https://github.com/chloro-pn/draw_io_repo/blob/master/tunnel.svg)
## Tunnel
Tunnel是一个跨平台、轻量级、适配性强的基于c++20 coroutine的任务执行框架，你可以用它构建具有复杂依赖关系的task执行引擎，或者pipeline执行引擎。
本项目的思路来源于ClickHouse的执行引擎，不同点在于本项目可以方便的与其他异步系统或者网络io进行集成，例如你可以在执行过程中等待第三方系统的某个异步操作，或者等待socket读写，这一切都不会阻塞线程。
本项目支持在task间传递参数，但是一个Pipeline只支持一种参数类型，如果您需要在不同的task间传递不同类型的数据，请使用std::any或者void*。

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
* `Sink`：`Sink`是一种`Processor`，它不具有`output_port`，是消费数据的节点。
* `DumpSimk`：`DumpSimk`是一种`Sink`，它读取并丢弃数据。
* `TransForm`：`TransForm`是一种`Processor`，它的存在仅为了提供一种不同于`Source`和`Sink`的`Processor`类型。
* `SimpleTransForm`：`SimpleTransForm`是一种`TransForm`，它只具有一个`input_port`和一个`output_port`，用于执行简单的转换，用户的大部分逻辑应该通过继承此类完成。
* `NoOpTransform`：`NoOpTransform`是一种`SimpleTransForm`，它什么都不做，一般用作占位节点。
* `Concat`：`Concat`是一种`Processor`，它具有一个或多个`input_port`和一个`output_port`，它总是按照顺序依次消费`input_port`并写入`output_port`。
* `Dispatch`：`Dispatch`是一种`Processor`，它具有一个`input_port`和一个或多个`output_port`，它执行分流功能，即根据用户设置的逻辑将`input_port`分发给不同的`output_port`。
* `Filter`：`Filter`是一种`TransForm`，它执行过滤功能，即根据用户设置的逻辑过滤掉`input_port`中的数据。
* `Fork`：`Fork`是一种`Processor`，它具有一个`input_port`和一个或多个`output_port`，它执行复制功能，即将`input_port`中的数据复制给每个`output_port`。

**注意**：本项目没有`Merge`节点，而是通过其他方法实现`Merge`功能，原因是`Merge`节点需要多个`input_port`，但是我们没有办法知道哪个`input_port`当前有数据到来，因此需要挂起等待某个`input_port`，这是不合理的。本项目通过共享多个`port`的队列来满足此功能，详情见`pipeline`的`Merge`接口。

---
节点类型的继承关系如下，标为红色的类型表示需要继承实现，标为蓝色的类型表示可以直接使用：
![node_type](https://github.com/chloro-pn/draw_io_repo/blob/master/nodes.svg)

## Doc
请阅读doc目录和example目录学习本项目的api使用。

## Todo
1. 支持更多类型的节点
2. 支持Pipeline合并
3. 拓扑检测
4. 调度事件收集
5. 支持中止执行
6. 执行过程中的异常处理
7. 支持运行时扩展Pipeline
8. 支持分布式调度（首先需要支持基于coroutine的网络io）


## License
本项目基于Apache License (Version 2.0) 协议。
