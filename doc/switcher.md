## switcher
switcher是tunnel中的协调组件，用来为不同进程、不同计算节点上的pipeline提供数据交换的能力。

## design
* switcher不需要是唯一的
* switcher是无状态的
* switcher是基于topic的
* switcher是惰性的-防止反压
* sdk是异步的-便于封装为协程
