/**
 * @file main.cpp
 * @brief Entry point: an SDL3 window, drawn with SDL_Renderer, until Esc or
 *        the window is closed.
 *
 * SDL3 is a C library, so its header is used as-is. core2 handles the
 * arguments and the messages on the console.
 */

#include <SDL3/SDL.h>

import core;

using namespace base;

namespace {

  constexpr i32 WINDOW_W = 960;   ///< Initial window width, in pixels.
  constexpr i32 WINDOW_H = 540;   ///< Initial window height, in pixels.
  constexpr f32 BOX = 64.0f;      ///< Side of the moving square, in pixels.
  constexpr f32 SPEED = 240.0f;   ///< The square's speed, in pixels per second.

  /**
   * @brief Where the square is after @p t seconds: back and forth across a
   *        track @p track pixels long.
   * @param t     Seconds since the start.
   * @param track Length of the track (the output width minus the square).
   * @return The square's left edge, in [0, @p track].
   */
  [[nodiscard]] auto bounce(f64 t, f32 track) -> f32 {
    if (track <= 0.0f) {
      return 0.0f;
    }
    f64 period = 2.0 * static_cast<f64>(track);
    f64 d = t * static_cast<f64>(SPEED);
    d -= period * static_cast<f64>(static_cast<u64>(d / period));
    return static_cast<f32>(d <= static_cast<f64>(track) ? d : period - d);
  }

}

auto main(int argc, char** argv) -> int {
  os::Args args = os::args(argc, argv);

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    console::eprintln("SDL_Init failed: {}", SDL_GetError());
    return 1;
  }

  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  if (!SDL_CreateWindowAndRenderer("core2 + SDL3", WINDOW_W, WINDOW_H, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    console::eprintln("SDL_CreateWindowAndRenderer failed: {}", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  // Without vsync the loop sleeps instead, so it does not spin a core.
  bool vsync = SDL_SetRenderVSync(renderer, 1);

  console::println("{}: {} argument(s); Esc or close the window to quit", os::arg(args, 0), args.count - 1);

  u64 start = SDL_GetTicksNS();
  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        running = false;
      }
    }

    i32 w = 0;
    i32 h = 0;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    f64 t = static_cast<f64>(SDL_GetTicksNS() - start) / 1e9;

    SDL_SetRenderDrawColor(renderer, 24, 26, 32, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    SDL_FRect box = {bounce(t, static_cast<f32>(w) - BOX), (static_cast<f32>(h) - BOX) / 2.0f, BOX, BOX};
    SDL_SetRenderDrawColor(renderer, 230, 140, 40, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &box);

    SDL_RenderPresent(renderer);
    if (!vsync) {
      SDL_Delay(16);
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
