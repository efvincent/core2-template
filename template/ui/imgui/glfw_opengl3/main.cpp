/**
 * @file main.cpp
 * @brief Entry point: Dear ImGui on GLFW, drawn with OpenGL 3.
 *
 * Follows Dear ImGui's `examples/example_glfw_opengl3` (v1.92.8, MIT), in
 * core2's style. GLFW and ImGui headers are used as-is (GLFW brings in the
 * system OpenGL header; ImGui's OpenGL 3 backend loads the functions it needs
 * itself); core2 handles the arguments and the console.
 */

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

import core;

using namespace base;

namespace {

  constexpr i32 WINDOW_W = 1280;  ///< Initial window width, before display scaling.
  constexpr i32 WINDOW_H = 800;   ///< Initial window height, before display scaling.

  /**
   * @brief GLFW's error callback: reports on stderr.
   * @param code GLFW's error code.
   * @param what The description.
   */
  auto glfwError(int code, const char* what) -> void {
    console::eprintln("GLFW error {}: {}", code, what);
  }

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

  glfwSetErrorCallback(glfwError);
  if (glfwInit() == GLFW_FALSE) {
    return 1;
  }

  // OpenGL 3.0 and GLSL 1.30, as in ImGui's example for Linux.
  const char* glsl = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  f32 scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
  if (scale <= 0.0f) {
    scale = 1.0f;
  }
  GLFWwindow* window = glfwCreateWindow(static_cast<i32>(static_cast<f32>(WINDOW_W) * scale), static_cast<i32>(static_cast<f32>(WINDOW_H) * scale),
                                        "core2 + Dear ImGui (GLFW, OpenGL 3)", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl);

  console::println("{}: {} argument(s); close the window to quit", os::arg(args, 0), args.count - 1);

  Ui ui = {ImVec4(0.45f, 0.55f, 0.60f, 1.00f), 0, true};
  while (glfwWindowShouldClose(window) == GLFW_FALSE) {
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    drawUi(ui, args, io);
    ImGui::Render();

    i32 w = 0;
    i32 h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(ui.clear.x * ui.clear.w, ui.clear.y * ui.clear.w, ui.clear.z * ui.clear.w, ui.clear.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
