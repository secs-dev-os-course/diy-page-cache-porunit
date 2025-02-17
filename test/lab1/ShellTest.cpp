#include <gtest/gtest.h>

#include "lab1/Shell.hpp"

namespace lab1 {

// Test to ensure command parsing splits the command and arguments correctly
TEST(Shell, CommandParsing) {
  auto args = ParseCommand("ls -lah");
  ASSERT_EQ(args.size(), 2);
  ASSERT_EQ(args[0], "ls");
  ASSERT_EQ(args[1], "-lah");

  args = ParseCommand("grep 'hello world' filename.txt");
  ASSERT_EQ(args.size(), 3);
  ASSERT_EQ(args[0], "grep");
  ASSERT_EQ(args[1], "hello world");
  ASSERT_EQ(args[2], "filename.txt");

  args = ParseCommand("echo \"hello world\"");
  ASSERT_EQ(args.size(), 2);
  ASSERT_EQ(args[0], "echo");
  ASSERT_EQ(args[1], "hello world");
}

}  // namespace lab1
