![tunnel icon](https://github.com/chloro-pn/draw_io_repo/blob/master/tunnel.svg)
## Tunnel | [中文](./README_CN.md)
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

## Design
Firstly, you need to understand several basic concepts:

* **`Processor`**: `Processor` is the basic unit for scheduling execution, and each `Processor` holds 0, 1, or more `input_port` and 0, 1, or more `output_port`. But it will not hold 0 `input_port` and 0 `output_port` at the same time.

* **`port`**：`port` is a tool for transferring data between `Processor`, and some `ports` share the same queue. `port` and are divided into `input_port` and `output_port`, `input_port` reads data from the queue, and `output_port` writes data to the queue.

* **`pipeline`**：a `pipeline` is composed of multiple `processors`. These `processors` are connected through `port` and have the structure of a Directed Acyclic Graph. The `pipeline` can be sent to the `Executor` for scheduling and execution.

* **`Executor`**：the `Executor` concept in `async_simple`.

The above are the four most basic concepts in this project, followed by some derived concepts:
* `Source`：`Source` is a type of `Processor` that does not have an `input_port` and is the node that generates data.
* `EmptySource`：`Source` is a type of `Processor` that only generates a EOF info.
* `Sink`：`Sink` is a type of `Processor` that does not have an `output_port` and is a node that consumes data.
* `DumpSimk`：`DumpSimk` is a type of `Sink` that reads and discards data.
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
Please read the doc directory and example directory to learn the API usage for this project.

## Todo
1. Support for more types of nodes [**doing**]
2. Support Pipeline Merge [**doing**]
3. Topology detection
4. Schedule event collection
5. Support active stop of execution
6. Exception handling during execution [**doing**]
7. Support for runtime extension of Pipeline
8. Support for distributed scheduling (support for network io based on async_simple first)


## License
tunnel is distributed under the Apache License (Version 2.0).
