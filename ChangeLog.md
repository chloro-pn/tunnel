## ChangeLog

##### [2023-7-16]
* 重构 Channel记录它的索引值，便于事件收集
* Processor的Pop和Push接口中收集一些信息，可以统计每个Processor等待在io上的时间。后续收集的信息会更加细化，例如thread_id，每个port上每次io的字节数等等。
* 后续也会支持收集用户自定义事件。