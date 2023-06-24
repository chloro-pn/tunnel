#include "gtest/gtest.h"
#include "tunnel/processor.h"

using namespace tunnel;

TEST(ProcessorTest, basic) {
  Processor<int> node;
  Processor<int> node2;
  connect(node, node2);
  EXPECT_EQ(node2.GetInputPort(), true);
  EXPECT_EQ(node.GetOutputPort(), true);
}