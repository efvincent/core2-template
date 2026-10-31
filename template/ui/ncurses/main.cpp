/**
 * @file main.cpp
 * @brief Entry point: shows the program's arguments in a curses screen.
 *
 * curses owns the terminal between `initscr` and `endwin`; nothing goes
 * through `console::print` in that window. Without a terminal on stdin and
 * stdout (piped, or run by a script), there is no screen to draw on, so the
 * arguments are printed through core2 instead.
 */

#include <ncurses.h>

import core;

using namespace base;

namespace {

  /**
   * @brief Draws @p s at row @p y, column @p x of the curses screen.
   * @param y Row.
   * @param x Column.
   * @param s The bytes to draw (need not be NUL-terminated).
   */
  auto drawStr(i32 y, i32 x, Str s) -> void {
    mvprintw(y, x, "%.*s", static_cast<int>(s.len), reinterpret_cast<const char*>(s.data));
  }

  /**
   * @brief Prints the arguments without curses.
   * @param args `main`'s arguments.
   */
  auto printPlain(os::Args args) -> void {
    console::println("{}: {} argument(s) (no terminal, so no curses screen)", os::arg(args, 0), args.count - 1);
    for (u64 i = 1; i < args.count; ++i) {
      console::println("  {}: {}", i, os::arg(args, i));
    }
  }

}

auto main(int argc, char** argv) -> int {
  os::Args args = os::args(argc, argv);

  if (!file::isTerminal(file::STDIN) || !file::isTerminal(file::STDOUT)) {
    printPlain(args);
    return 0;
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  drawStr(1, 2, os::arg(args, 0));
  mvprintw(2, 2, "%llu argument(s)", static_cast<unsigned long long>(args.count - 1));
  i32 row = 4;
  for (u64 i = 1; i < args.count && row < LINES - 2; ++i, ++row) {
    mvprintw(row, 4, "%llu: ", static_cast<unsigned long long>(i));
    drawStr(row, 9, os::arg(args, i));
  }
  mvprintw(LINES - 1, 2, "Press any key to exit");
  refresh();
  getch();

  endwin();
  return 0;
}
