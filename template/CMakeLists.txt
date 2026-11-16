# {{project-name}}
#
# Built on core2 (third-party/core2, a git submodule pinned to a release tag),
# through its CMake target: add_subdirectory builds core2 from the submodule's
# working tree as part of this build (so an edit in third-party/core2 is
# picked up by the next build here), and core2::core2 brings core2's required
# flags with it.
#
#   cmake --preset debug && cmake --build --preset debug
#   ./build/cmake/debug/{{project-name}}
#
# Presets (CMakePresets.json): debug, release, lto, instrument. Each needs its
# `cmake --preset <name>` once, builds into build/cmake/<preset>/ and writes
# compile_commands.json there, for clangd. -DNDEBUG comes only from the
# presets' build types, which core2 shares.

cmake_minimum_required(VERSION 3.30...4.4)
project({{project-name}} LANGUAGES CXX)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/third-party/core2/CMakeLists.txt")
  message(FATAL_ERROR "third-party/core2 is not checked out (or predates core2's CMake build, v1.1.0): run `git submodule update --init` (in a freshly generated project, `sh setup.sh`)")
endif()

add_subdirectory(third-party/core2)
{%- if ui != "plain" %}

# The UI libraries, through pkg-config. Their compile flags reach only the
# targets that link PkgConfig::ui, never core2.
set(ui__packages {% if ui == "ncurses" %}ncursesw{% elsif ui == "sdl3" %}sdl3{% elsif imgui_backend == "sdl3_renderer" or imgui_backend == "sdl3_gpu" %}sdl3{% elsif imgui_backend == "glfw_opengl3" %}glfw3 gl{% else %}glfw3 vulkan{% endif %})
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(ui QUIET IMPORTED_TARGET ${ui__packages})
endif()
if(NOT ui_FOUND)
  message(FATAL_ERROR "pkg-config cannot find ${ui__packages}: install their development files (see README.md)")
endif()
{%- if imgui_backend == "glfw_vulkan" %}

# The loader's vulkan.pc does not bring the headers; they are a package of their own.
find_path(ui__vulkan_headers vulkan/vulkan.h HINTS ${ui_INCLUDE_DIRS})
if(NOT ui__vulkan_headers)
  message(FATAL_ERROR "vulkan/vulkan.h not found: install the Vulkan headers (Arch: vulkan-headers; Debian, Ubuntu: libvulkan-dev; Fedora: vulkan-headers)")
endif()
{%- endif %}
{%- endif %}
{%- if ui == "imgui" %}

# Dear ImGui (third-party/imgui, a submodule pinned to v1.92.8): the library
# and its {{imgui_backend}} backends. It does not import core2, so it gets
# none of core2's flags and none of this project's warnings (it is not this
# project's code): -fno-threadsafe-statics keeps it off the C++ runtime's
# __cxa_guard_*. The build type's flags (optimization, LTO, sanitizers) apply
# to it as to everything else. This project's sources see its headers as
# system headers.
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/third-party/imgui/imgui.h")
  message(FATAL_ERROR "third-party/imgui is not checked out: run `git submodule update --init` (in a freshly generated project, `sh setup.sh`)")
endif()

set(imgui__dir "${PROJECT_SOURCE_DIR}/third-party/imgui")
add_library(imgui STATIC
  ${imgui__dir}/imgui.cpp ${imgui__dir}/imgui_draw.cpp ${imgui__dir}/imgui_tables.cpp
  ${imgui__dir}/imgui_widgets.cpp ${imgui__dir}/imgui_demo.cpp
  {% if imgui_backend == "sdl3_renderer" %}${imgui__dir}/backends/imgui_impl_sdl3.cpp ${imgui__dir}/backends/imgui_impl_sdlrenderer3.cpp{% elsif imgui_backend == "sdl3_gpu" %}${imgui__dir}/backends/imgui_impl_sdl3.cpp ${imgui__dir}/backends/imgui_impl_sdlgpu3.cpp{% elsif imgui_backend == "glfw_opengl3" %}${imgui__dir}/backends/imgui_impl_glfw.cpp ${imgui__dir}/backends/imgui_impl_opengl3.cpp{% else %}${imgui__dir}/backends/imgui_impl_glfw.cpp ${imgui__dir}/backends/imgui_impl_vulkan.cpp{% endif %})
set_target_properties(imgui PROPERTIES CXX_SCAN_FOR_MODULES OFF)
target_compile_options(imgui PRIVATE -fno-exceptions -fno-rtti -fno-threadsafe-statics)
target_include_directories(imgui SYSTEM PUBLIC ${imgui__dir})
target_link_libraries(imgui PUBLIC PkgConfig::ui)
{%- endif %}

# The project: every src/**/*.cpp (each may `import core;`) and the project's
# modules, src/**/*.cppm.
file(GLOB_RECURSE app__modules CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cppm")
file(GLOB_RECURSE app__sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable({{project-name}})
target_sources({{project-name}}
  PRIVATE ${app__sources}
  PRIVATE FILE_SET CXX_MODULES BASE_DIRS src FILES ${app__modules})
target_include_directories({{project-name}} PRIVATE src)
target_compile_options({{project-name}} PRIVATE
  -Wall -Wextra -Wpedantic -Wglobal-constructors -Wexit-time-destructors)
target_link_libraries({{project-name}} PRIVATE core2::core2{% if ui == "imgui" %} imgui{% elsif ui != "plain" %} PkgConfig::ui{% endif %})

# The reference (Doxygen, docs/Doxyfile): docs/html/index.html. Fails on any
# Doxygen warning. Defined only when doxygen is installed.
find_program(DOXYGEN_EXECUTABLE doxygen)
if(DOXYGEN_EXECUTABLE)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} docs/Doxyfile
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    USES_TERMINAL)
endif()
