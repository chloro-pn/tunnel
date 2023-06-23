#include "tunnel/processor.h"
#include "gtest/gtest.h"

using namespace tunnel;

TEST(ProcessorTest, basic) {
  Processor<int> node;
  Processor<int> node2;
  connect(node, node2);
  EXPECT_EQ(node2.GetInputPorts().size(), 1);
  EXPECT_EQ(node.GetOutputPorts().size(), 1);
}