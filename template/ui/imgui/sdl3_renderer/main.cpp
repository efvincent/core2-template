/**
 * @file main.cpp
 * @brief Entry point: Dear ImGui on SDL3, drawn with SDL_Renderer.
 *
 * Follows Dear ImGui's `examples/example_sdl3_sdlrenderer3` (v1.92.8, MIT),
 * in core2's style. SDL3 and ImGui headers are used as-is; core2 handles the
 * arguments and the console.
 */

#include <SDL3/SDL.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

import core;

using namespace base;

namespace {

  constexpr i32 WINDOW_W = 1280;  ///< Initial window width, before display scaling.
  constexpr i32 WINDOW_H = 800;   ///< Initial window height, before display scaling.

  /**
   * @brief What the windows show and edit, kept from frame to frame.
   */
  struct Ui {
    ImVec4 clear;   ///< Background color.
    i32 clicks;     ///< Times the button was pressed.
    bool showDemo;  ///< Whether ImGui's demo window is open.
  };

  /**
   * @brief Builds this frame's windows.
   * @param ui   The state the widgets edit.
   * @param args `main`'s arguments, listed in the window.
   * @param io   ImGui's IO, for the frame rate.
   */
  auto drawUi(Ui& ui, os::Args args, const ImGuiIO& io) -> void {
    ImGui::Begin("core2 + Dear ImGui");
    ImGui::Text("%llu argument(s)", static_cast<unsigned long long>(args.count - 1));
    for (u64 i = 1; i < args.count; ++i) {
      Str a = os::arg(args, i);
      ImGui::BulletText("%.*s", static_cast<int>(a.len), reinterpret_cast<const char*>(a.data));
    }
    ImGui::Separator();
    ImGui::Checkbox("Demo window", &ui.showDemo);
    ImGui::ColorEdit3("Background", &ui.clear.x);
    if (ImGui::Button("Click")) {
      ++ui.clicks;
    }
    ImGui::SameLine();
    ImGui::Text("clicks = %d", ui.clicks);
    ImGui::Text("%.3f ms/frame (%.1f FPS)", static_cast<f64>(1000.0f / io.Framerate), static_cast<f64>(io.Framerate));
    ImGui::End();

    if (ui.showDemo) {
      ImGui::ShowDemoWindow(&ui.showDemo);
    }
  }

}

auto main(int argc, char** argv) -> int {
  os::Args args = os::args(argc, argv);

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    console::eprintln("SDL_Init failed: {}", SDL_GetError());
    return 1;
  }

  f32 scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  if (scale <= 0.0f) {
    scale = 1.0f;
  }
  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  SDL_Window* window = SDL_CreateWindow("core2 + Dear ImGui (SDL3, SDL_Renderer)", static_cast<i32>(static_cast<f32>(WINDOW_W) * scale),
                                        static_cast<i32>(static_cast<f32>(WINDOW_H) * scale), flags);
  if (window == nullptr) {
    console::eprintln("SDL_CreateWindow failed: {}", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
  if (renderer == nullptr) {
    console::eprintln("SDL_CreateRenderer failed: {}", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_SetRenderVSync(renderer, 1);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  console::println("{}: {} argument(s); close the window to quit", os::arg(args, 0), args.count - 1);

  Ui ui = {ImVec4(0.45f, 0.55f, 0.60f, 1.00f), 0, true};
  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
        running = false;
      }
    }
    if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0) {
      SDL_Delay(10);
      continue;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    drawUi(ui, args, io);
    ImGui::Render();

    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(renderer, ui.clear.x, ui.clear.y, ui.clear.z, ui.clear.w);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
