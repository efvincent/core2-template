/**
 * @file main.cpp
 * @brief Entry point: Dear ImGui on GLFW, drawn with Vulkan.
 *
 * Follows Dear ImGui's `examples/example_glfw_vulkan` (v1.92.8, MIT), in
 * core2's style: the example's globals are one `Vulkan` struct on `main`'s
 * stack, and Vulkan errors stop the program through `trap::panic`. GLFW,
 * Vulkan and ImGui headers are used as-is; core2 handles the arguments and
 * the console. The swapchain, render pass and per-frame objects come from
 * ImGui's `ImGui_ImplVulkanH_*` helpers, as in the example; a larger program
 * would own them.
 */

#include <string.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

import core;

using namespace base;

namespace {

  constexpr i32 WINDOW_W = 1280;     ///< Initial window width, before display scaling.
  constexpr i32 WINDOW_H = 800;      ///< Initial window height, before display scaling.
  constexpr u32 MIN_IMAGE_COUNT = 2; ///< Fewest swapchain images to ask for.

  /**
   * @brief Everything Vulkan the program holds, from setup to cleanup.
   */
  struct Vulkan {
    ImGui_ImplVulkanH_Window window;  ///< Surface, swapchain, render pass and frames (ImGui's helper).
    VkInstance instance;              ///< The instance.
    VkPhysicalDevice gpu;             ///< The chosen physical device.
    VkDevice device;                  ///< The logical device.
    VkQueue queue;                    ///< The graphics queue (also used to present).
    VkDescriptorPool pool;            ///< Descriptors for ImGui's textures.
    u32 queueFamily;                  ///< The graphics queue's family.
    bool rebuild;                     ///< The swapchain is out of date and must be rebuilt.
  };

  /**
   * @brief GLFW's error callback: reports on stderr.
   * @param code GLFW's error code.
   * @param what The description.
   */
  auto glfwError(int code, const char* what) -> void {
    console::eprintln("GLFW error {}: {}", code, what);
  }

  /**
   * @brief Stops the program on a Vulkan error; reports a positive status.
   *        Also ImGui's `CheckVkResultFn`.
   * @param err The result to check.
   */
  auto checkVk(VkResult err) -> void {
    if (err == VK_SUCCESS) {
      return;
    }
    if (err < 0) {
      trap::panic("Vulkan error: VkResult %d", static_cast<int>(err));
    }
    console::eprintln("Vulkan: VkResult {}", static_cast<i32>(err));
  }

  /**
   * @brief Whether @p name is among @p props.
   * @param props Extension properties, as Vulkan lists them.
   * @param name  The extension's name.
   * @return True when it is there.
   */
  [[nodiscard]] auto hasExtension(const ImVector<VkExtensionProperties>& props, const char* name) -> bool {
    for (const VkExtensionProperties& p : props) {
      if (strcmp(p.extensionName, name) == 0) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Creates the instance, picks the device and queue, creates the
   *        logical device and the descriptor pool.
   * @param vk   Where the handles go.
   * @param exts The instance extensions GLFW needs; more are added here.
   */
  auto setupVulkan(Vulkan& vk, ImVector<const char*> exts) -> void {
    VkInstanceCreateInfo instance = {};
    instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    u32 count = 0;
    ImVector<VkExtensionProperties> props;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    props.resize(static_cast<int>(count));
    checkVk(vkEnumerateInstanceExtensionProperties(nullptr, &count, props.Data));
    if (hasExtension(props, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
      exts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    // Portability drivers (MoltenVK) are listed only when asked for.
    if (hasExtension(props, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
      exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      instance.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif
    instance.enabledExtensionCount = static_cast<u32>(exts.Size);
    instance.ppEnabledExtensionNames = exts.Data;
    checkVk(vkCreateInstance(&instance, nullptr, &vk.instance));

    vk.gpu = ImGui_ImplVulkanH_SelectPhysicalDevice(vk.instance);
    if (vk.gpu == VK_NULL_HANDLE) {
      trap::panic("Vulkan: no physical device");
    }
    vk.queueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(vk.gpu);
    if (vk.queueFamily == static_cast<u32>(-1)) {
      trap::panic("Vulkan: no graphics queue family");
    }

    ImVector<const char*> deviceExts;
    deviceExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    vkEnumerateDeviceExtensionProperties(vk.gpu, nullptr, &count, nullptr);
    props.resize(static_cast<int>(count));
    vkEnumerateDeviceExtensionProperties(vk.gpu, nullptr, &count, props.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    // A beta extension: defined only with VK_ENABLE_BETA_EXTENSIONS.
    if (hasExtension(props, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
      deviceExts.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
#endif
    const f32 priority = 1.0f;
    VkDeviceQueueCreateInfo queue = {};
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.queueFamilyIndex = vk.queueFamily;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    VkDeviceCreateInfo device = {};
    device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device.queueCreateInfoCount = 1;
    device.pQueueCreateInfos = &queue;
    device.enabledExtensionCount = static_cast<u32>(deviceExts.Size);
    device.ppEnabledExtensionNames = deviceExts.Data;
    checkVk(vkCreateDevice(vk.gpu, &device, nullptr, &vk.device));
    vkGetDeviceQueue(vk.device, vk.queueFamily, 0, &vk.queue);

    VkDescriptorPoolSize sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE},
      {VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE},
    };
    VkDescriptorPoolCreateInfo pool = {};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    for (const VkDescriptorPoolSize& s : sizes) {
      pool.maxSets += s.descriptorCount;
    }
    pool.poolSizeCount = static_cast<u32>(IM_COUNTOF(sizes));
    pool.pPoolSizes = sizes;
    checkVk(vkCreateDescriptorPool(vk.device, &pool, nullptr, &vk.pool));
  }

  /**
   * @brief Picks the surface format and present mode, and creates the
   *        swapchain and its frames.
   * @param vk      The Vulkan state; its `window` is filled in.
   * @param surface The window's surface.
   * @param w       Framebuffer width.
   * @param h       Framebuffer height.
   * @return False when the device cannot present to @p surface.
   */
  [[nodiscard]] auto setupWindow(Vulkan& vk, VkSurfaceKHR surface, i32 w, i32 h) -> bool {
    VkBool32 present = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(vk.gpu, vk.queueFamily, surface, &present);
    if (present != VK_TRUE) {
      return false;
    }
    ImGui_ImplVulkanH_Window* wd = &vk.window;
    wd->Surface = surface;
    const VkFormat formats[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(vk.gpu, surface, formats, IM_COUNTOF(formats), VK_COLORSPACE_SRGB_NONLINEAR_KHR);
    const VkPresentModeKHR modes[] = {VK_PRESENT_MODE_FIFO_KHR};  // vsync
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(vk.gpu, surface, modes, IM_COUNTOF(modes));
    ImGui_ImplVulkanH_CreateOrResizeWindow(vk.instance, vk.gpu, vk.device, wd, vk.queueFamily, nullptr, w, h, MIN_IMAGE_COUNT, 0);
    return true;
  }

  /**
   * @brief Records and submits one frame's commands.
   * @param vk   The Vulkan state; `rebuild` is set when the swapchain is out of date.
   * @param draw ImGui's draw data for the frame.
   */
  auto renderFrame(Vulkan& vk, ImDrawData* draw) -> void {
    ImGui_ImplVulkanH_Window* wd = &vk.window;
    VkSemaphore acquired = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore done = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(vk.device, wd->Swapchain, UINT64_MAX, acquired, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
      vk.rebuild = true;
    }
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
      return;
    }
    if (err != VK_SUBOPTIMAL_KHR) {
      checkVk(err);
    }

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    checkVk(vkWaitForFences(vk.device, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
    checkVk(vkResetFences(vk.device, 1, &fd->Fence));
    checkVk(vkResetCommandPool(vk.device, fd->CommandPool, 0));

    VkCommandBufferBeginInfo begin = {};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkVk(vkBeginCommandBuffer(fd->CommandBuffer, &begin));

    VkRenderPassBeginInfo pass = {};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = wd->RenderPass;
    pass.framebuffer = fd->Framebuffer;
    pass.renderArea.extent.width = static_cast<u32>(wd->Width);
    pass.renderArea.extent.height = static_cast<u32>(wd->Height);
    pass.clearValueCount = 1;
    pass.pClearValues = &wd->ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &pass, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(draw, fd->CommandBuffer);
    vkCmdEndRenderPass(fd->CommandBuffer);
    checkVk(vkEndCommandBuffer(fd->CommandBuffer));

    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &acquired;
    submit.pWaitDstStageMask = &stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &fd->CommandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &done;
    checkVk(vkQueueSubmit(vk.queue, 1, &submit, fd->Fence));
  }

  /**
   * @brief Presents the frame `renderFrame` submitted.
   * @param vk The Vulkan state; `rebuild` is set when the swapchain is out of date.
   */
  auto presentFrame(Vulkan& vk) -> void {
    if (vk.rebuild) {
      return;
    }
    ImGui_ImplVulkanH_Window* wd = &vk.window;
    VkSemaphore done = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &done;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(vk.queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
      vk.rebuild = true;
    }
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
      return;
    }
    if (err != VK_SUBOPTIMAL_KHR) {
      checkVk(err);
    }
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
  }

  /**
   * @brief Destroys what `setupWindow` and `setupVulkan` created, in reverse.
   * @param vk The Vulkan state.
   */
  auto cleanupVulkan(Vulkan& vk) -> void {
    ImGui_ImplVulkanH_DestroyWindow(vk.instance, vk.device, &vk.window, nullptr);
    vkDestroySurfaceKHR(vk.instance, vk.window.Surface, nullptr);
    vkDestroyDescriptorPool(vk.device, vk.pool, nullptr);
    vkDestroyDevice(vk.device, nullptr);
    vkDestroyInstance(vk.instance, nullptr);
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
  if (glfwVulkanSupported() == GLFW_FALSE) {
    console::eprintln("GLFW: Vulkan is not supported");
    glfwTerminate();
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  f32 scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
  if (scale <= 0.0f) {
    scale = 1.0f;
  }
  GLFWwindow* window = glfwCreateWindow(static_cast<i32>(static_cast<f32>(WINDOW_W) * scale), static_cast<i32>(static_cast<f32>(WINDOW_H) * scale),
                                        "core2 + Dear ImGui (GLFW, Vulkan)", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return 1;
  }

  ImVector<const char*> exts;
  u32 extCount = 0;
  const char** glfwExts = glfwGetRequiredInstanceExtensions(&extCount);
  for (u32 i = 0; i < extCount; ++i) {
    exts.push_back(glfwExts[i]);
  }
  Vulkan vk = {};
  vk.queueFamily = static_cast<u32>(-1);
  setupVulkan(vk, exts);

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  checkVk(glfwCreateWindowSurface(vk.instance, window, nullptr, &surface));
  i32 w = 0;
  i32 h = 0;
  glfwGetFramebufferSize(window, &w, &h);
  if (!setupWindow(vk, surface, w, h)) {
    console::eprintln("Vulkan: the device cannot present to this window");
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_InitInfo info = {};
  info.Instance = vk.instance;
  info.PhysicalDevice = vk.gpu;
  info.Device = vk.device;
  info.QueueFamily = vk.queueFamily;
  info.Queue = vk.queue;
  info.DescriptorPool = vk.pool;
  info.MinImageCount = MIN_IMAGE_COUNT;
  info.ImageCount = vk.window.ImageCount;
  info.PipelineInfoMain.RenderPass = vk.window.RenderPass;
  info.PipelineInfoMain.Subpass = 0;
  info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  info.CheckVkResultFn = checkVk;
  ImGui_ImplVulkan_Init(&info);

  console::println("{}: {} argument(s); close the window to quit", os::arg(args, 0), args.count - 1);

  Ui ui = {ImVec4(0.45f, 0.55f, 0.60f, 1.00f), 0, true};
  while (glfwWindowShouldClose(window) == GLFW_FALSE) {
    glfwPollEvents();

    // A resize or an out-of-date swapchain: rebuild it at the new size.
    i32 fbW = 0;
    i32 fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    if (fbW > 0 && fbH > 0 && (vk.rebuild || vk.window.Width != fbW || vk.window.Height != fbH)) {
      ImGui_ImplVulkan_SetMinImageCount(MIN_IMAGE_COUNT);
      ImGui_ImplVulkanH_CreateOrResizeWindow(vk.instance, vk.gpu, vk.device, &vk.window, vk.queueFamily, nullptr, fbW, fbH, MIN_IMAGE_COUNT, 0);
      vk.window.FrameIndex = 0;
      vk.rebuild = false;
    }
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    drawUi(ui, args, io);
    ImGui::Render();

    ImDrawData* draw = ImGui::GetDrawData();
    if (draw->DisplaySize.x > 0.0f && draw->DisplaySize.y > 0.0f) {
      vk.window.ClearValue.color.float32[0] = ui.clear.x * ui.clear.w;
      vk.window.ClearValue.color.float32[1] = ui.clear.y * ui.clear.w;
      vk.window.ClearValue.color.float32[2] = ui.clear.z * ui.clear.w;
      vk.window.ClearValue.color.float32[3] = ui.clear.w;
      renderFrame(vk, draw);
      presentFrame(vk);
    }
  }

  checkVk(vkDeviceWaitIdle(vk.device));
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  cleanupVulkan(vk);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
