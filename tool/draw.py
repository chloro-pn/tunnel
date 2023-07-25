# based on Graphviz

import graphviz
import sys

if len(sys.argv) != 2 : 
  print ("input pipeline dump file")
  exit(-1)

filename = sys.argv[1]

f = open(filename, encoding='utf-8')
pipeline_name = f.readline().split(' ')[2].split('\n')[0]
node_size = int(f.readline().split(' ')[3])

processor_map = {}
processor_name = {}

for i in range(node_size) : 
  line = f.readline().split('\n')[0]
  id = line.split(' ')[0]
  name = line.split(' ')[1]
  processor_name[id] = name[1:-1]
  processor_map[id] = line.split(' ')[3:-1]

graph = graphviz.Digraph(name = "pipeline", filename = pipeline_name+".gv", node_attr={'color' : 'lightblue2', 'style' : 'filled'})

for id in processor_name:
  graph.node(id, processor_name[id])

for id in processor_map:
  for post in processor_map[id]:
    graph.edge(id, post)

graph.render(directory = 'output')
