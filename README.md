## tunnel

Distributed pipeline execution engine based on async_simple

## design
每个Channl只允许一读一写、一读多写、多读一写三种模式中的一种，不允许多读多写（原因是难以准确传递eof信息）。

# todo:
* fork node
* filter node
* black hole sink