/**
 * @file main.cpp
 * @brief Entry point: Dear ImGui on SDL3, drawn with SDL_GPU.
 *
 * Follows Dear ImGui's `examples/example_sdl3_sdlgpu3` (v1.92.8, MIT),
 * in core2's style. SDL3 and ImGui headers are used as-is; core2 handles the
 * arguments and the console.
 */

#include <SDL3/SDL.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"

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
  SDL_Window* window = SDL_CreateWindow("core2 + Dear ImGui (SDL3, SDL_GPU)", static_cast<i32>(static_cast<f32>(WINDOW_W) * scale),
                                        static_cast<i32>(static_cast<f32>(WINDOW_H) * scale), flags);
  if (window == nullptr) {
    console::eprintln("SDL_CreateWindow failed: {}", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB;
  SDL_GPUDevice* gpu = SDL_CreateGPUDevice(formats, true, nullptr);
  if (gpu == nullptr) {
    console::eprintln("SDL_CreateGPUDevice failed: {}", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  if (!SDL_ClaimWindowForGPUDevice(gpu, window)) {
    console::eprintln("SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
    SDL_DestroyGPUDevice(gpu);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  ImGui_ImplSDL3_InitForSDLGPU(window);
  ImGui_ImplSDLGPU3_InitInfo info = {};
  info.Device = gpu;
  info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu, window);
  info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
  info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
  ImGui_ImplSDLGPU3_Init(&info);

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

    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    drawUi(ui, args, io);
    ImGui::Render();

    ImDrawData* draw = ImGui::GetDrawData();
    bool minimized = draw->DisplaySize.x <= 0.0f || draw->DisplaySize.y <= 0.0f;
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUTexture* swapchain = nullptr;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain, nullptr, nullptr);
    if (swapchain != nullptr && !minimized) {
      // Uploads the vertex and index buffers; must come before the render pass.
      ImGui_ImplSDLGPU3_PrepareDrawData(draw, cmd);

      SDL_GPUColorTargetInfo target = {};
      target.texture = swapchain;
      target.clear_color = SDL_FColor{ui.clear.x, ui.clear.y, ui.clear.z, ui.clear.w};
      target.load_op = SDL_GPU_LOADOP_CLEAR;
      target.store_op = SDL_GPU_STOREOP_STORE;
      SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
      ImGui_ImplSDLGPU3_RenderDrawData(draw, cmd, pass);
      SDL_EndGPURenderPass(pass);
    }
    SDL_SubmitGPUCommandBuffer(cmd);
  }

  SDL_WaitForGPUIdle(gpu);
  ImGui_ImplSDL3_Shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui::DestroyContext();
  SDL_ReleaseWindowFromGPUDevice(gpu, window);
  SDL_DestroyGPUDevice(gpu);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
