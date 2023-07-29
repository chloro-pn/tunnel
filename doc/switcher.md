## switcher
switcher是tunnel中的协调组件，用来为不同进程、不同计算节点上的pipeline提供数据交换的能力。

## design
* switcher不需要是唯一的
* switcher是无状态的
* switcher是基于topic的
* switcher不直接接收或者发送pipeline中传递的数据，只提供协调服务，即为一对目标为同一个topic的push方与pop方建立tcp通道提供服务。
* switcher是惰性的-防止反压
* sdk是异步的-便于封装为协程
* 每一对push与pop独占一条tcp通道（ap场景下对fd的消耗有限）
* sdk是进程内唯一的
