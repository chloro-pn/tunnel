## Pipeline

用户通过`Pipeline`持有和组装各种`Processor`节点，并交给`async_simple::Executor`调度。下面介绍关于`Pipeline`的基本概念和注意事项：
* 每个`Pipeline`必须具有`Sink`节点和`Source`节点，它们可以是一个或者多个；
* 本库给构造的每个`Processor`节点分配一个唯一id，`Pipeline`的大部分接口都需要通过id指定连接的或者构造的`Processor`节点。
* `Pipeline`在调度前需要对`Processor`节点进行拓扑检测，确保没有环、没有孤立节点等。（TODO）
* `Processor`会记录调度过程中的大部分事件，包括每个`Processor`节点什么时刻被挂起、什么时刻被回复、从`input_port`读了多少次、写`output_port`多少次等等，这些信息有助于排查问题和性能分析。（TODO）
* `Pipeline`的接口只允许为当前还没有设置`output_port`的节点（本库称之为叶节点，特别的，Sink节点虽然没有`output_port`节点，但它不属于叶节点）添加后置节点。（如果称节点B是节点A的后置节点，则意味着节点A的一个`output_port`和节点B的一个`input_port`连通）
* 叶节点数量为0的`Pipeline`才可以被调度。

#### interface
* `uint64_t AddSource(std::unique_ptr<Source<T>>&& source)`
为`Pipeline`新增加一个`Source`节点，返回值为该节点的id。

* `uint64_t AddTransform(uint64_t leaf_id, std::unique_ptr<Transform<T>>&& transform)`
为叶节点leaf_id添加一个后置节点，返回值为该后置节点的id。

* `std::unordered_set<uint64_t> AddTransform(const std::function<std::unique_ptr<Transform<T>>()>& creater)`
为当前所有的叶节点分别添加一个后置节点，该后置节点通过create()获得，返回值为这些后置节点的id。

* `uint64_t SetSink(std::unique_ptr<Sink<T>>&& sink)`
为当前所有的叶节点添加一个共享的后置Sink节点，即这些节点和Sink节点间使用的是同一条队列。

* `uint64_t SetSink(std::unique_ptr<Sink<T>>&& sink, const std::unordered_set<uint64_t>& leaves)`
为leavs指定的叶节点添加一个共享的后置Sink节点，即这些节点和Sink节点间使用的是同一条队列。

* `uint64_t Merge(std::unique_ptr<Transform<T>>&& transform, const std::unordered_set<uint64_t>& leaves)`
为leavs指定的叶节点添加一个共享的后置transform节点，这些节点和transform节点使用同一条队列，**注意**，这与`Concat`节点不同，`Concat`节点虽然也用来为多个叶节点添加一个共享的后置节点，但是这些节点与后置节点间的队列是独立的。

* `uint64_t Merge(const std::unordered_set<uint64_t>& leaves)`
同上，这个接口会生成一个NoOp节点来对leaves叶节点进行merge，返回值为NoOp节点的id。

* `uint64_t Merge(std::unique_ptr<Transform<T>>&& transform)`
同样是merge系列接口，这个接口会使用transform节点来对所有叶节点进行merge，返回值为transform的id。

* `uint64_t Merge()`
同样是merge系列接口，这个接口会生成一个NoOp节点来对所有叶节点进行merge，返回值为NoOp的id。

* `std::unordered_set<uint64_t> DispatchFrom(uint64_t leaf, std::unique_ptr<Dispatch<T>>&& node)`
这个接口首先将node设置为leaf的后置节点，然后根据node的分支数量（由用户指定）生成多个`output_port`，最后为每个`output_port`指定一个`NoOp`的后置节点，返回值为这些`NoOp`后置节点的id。
`Dispatch`节点可以用来做分流，根据用户指定的规则，将`input_port`中的数据分发给`output_port`中的一个。

* `uint64_t ConcatFrom(const std::vector<uint64_t>& leaves)`
这个接口为leaves叶节点添加一个共享的后置`Concat`节点，这些叶节点与`Concat`节点使用独立的队列。返回值为`Concat`节点的id。
`Concat`节点会按照指定顺序依次从leaves节点中读取数据，某个节点读到eof之后继续从下一个节点读。

* `std::unordered_set<uint64_t> ForkFrom(uint64_t leaf, size_t size)`
这个接口首先为leaf叶节点添加一个`Fork`后置节点，然后为`Fork`节点生成size个`output_port`，并为每个`output_port`添加一个NoOp节点，返回值为这些NoOp节点的id。
`Fork`节点用来执行复制，它会将读取的数据复制并传递给每个`output_port`。

* `Lazy<void> Run() &&`
启动pipeline。

* `bool IsCompleted() const`
返回pipeline是否是完整的。（我们将没有叶节点的pipeline称为完整的）

* `const std::unordered_set<uint64_t>& CurrentLeaves() const`
返回pipeline当前的叶节点id。