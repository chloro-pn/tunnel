## ChangeLog

##### [2023-7-15]
* 重构 Channel记录索引值，便于事件收集
* Processor的Pop和Push接口中收集一些信息，可以统计每个Processor等待在io上的时间。后续收集的信息会更加细化，例如thread_id，每个port上每次io的字节数等等。
* 后续也会支持收集用户自定义事件。

##### [2023-7-16]
* 重构 Processor的事件收集机制，收集的事件的统计结果作为Pipeline.Run函数的返回值返回给用户

##### [2023-7-17]
* new concept RecordTransferredBytes, 对于数据类型T，用户可以通过在tunnel命名空间添加`GetBytes`的T的特化版本，或者添加一个`size_t tunnel_get_bytes() const`成员函数来使统计信息中包含每个节点读取与写出的字节数。
* 关于中止信息如何跨pipeline传递的方案 [todo]

##### [2023-7-18]
* add new interface Pipeline.BindExecutorForProcessor

##### [2023-7-19]
* 支持多个pipeline绑定同一个abort_channel

##### [2023-7-20]
* 重构channel_sink和channel_source的托管模式，应该向用户设置的channel传递退出信息。

##### [2023-7-21]
* add new awaiter SocketAcceptAwaiter
* replace SimpleExecutor in some test with TunnelExecutor (fix occasionally core)

