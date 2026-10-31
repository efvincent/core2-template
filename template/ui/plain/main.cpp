/**
 * @file main.cpp
 * @brief Entry point: prints the program's arguments through core2.
 */

import core;

using namespace base;

auto main(int argc, char** argv) -> int {
  os::Args args = os::args(argc, argv);
  console::println("{}: {} argument(s)", os::arg(args, 0), args.count - 1);

  for (u64 i = 1; i < args.count; ++i) {
    console::println("  {}: {}", i, os::arg(args, i));
  }
  return 0;
}
