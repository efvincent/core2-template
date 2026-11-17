/**
 * @file main.cpp
 * @brief Entry point: prints the program's arguments through core2.
 */

import core;

using namespace base;

/**
 * @brief Prints the program's name and its arguments, one per line.
 * @param argc The argument count, as the C runtime passes it.
 * @param argv The arguments; `argv[0]` is the program's name.
 * @return Always 0.
 */
auto main(int argc, char** argv) -> int {
  os::Args args = os::args(argc, argv);
  console::println("{}: {} argument(s)", os::arg(args, 0), args.count - 1);

  for (u64 i = 1; i < args.count; ++i) {
    console::println("  {}: {}", i, os::arg(args, i));
  }
  return 0;
}
